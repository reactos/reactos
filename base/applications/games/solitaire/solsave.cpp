/* Versioned, validated per-user game persistence. */
#include "solitaire.h"
#include <winreg.h>

static const WCHAR settingsKey[] = L"Software\\ReactOS\\Solitaire";
static const DWORD savedVersion = 2;
static const DWORD ruleMask = OPTION_THREE_CARDS | OPTION_SCORE_STD | OPTION_SCORE_VEGAS | OPTION_KEEP_SCORE | OPTION_SHOW_TIME;
struct SavedGame
{
    DWORD version, rules, time, wasteCount, visible, started;
    LONG score;
    DWORD count[13]; // Stock, seven columns, four foundations, complete waste.
    DWORD card[13][52]; // Card index plus bit 8 for face-up.
    DWORD faceDown[NUM_ROW_STACKS];
    LONG roundBase;
    FILETIME roundId;
};

static CardRegion *SavedRegion(int i)
{
    if (!i) return pDeck;
    if (i <= NUM_ROW_STACKS) return pRowStack[i - 1];
    return pSuitStack[i - 1 - NUM_ROW_STACKS];
}

void SaveGame(void)
{
    if (!pDeck) return;
    HKEY key;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, settingsKey, 0, NULL, 0,
                        KEY_SET_VALUE, NULL, &key, NULL) != ERROR_SUCCESS)
        return;
    SavedGame state = {};
    state.version = savedVersion;
    state.rules = dwOptions & ruleMask;
    state.time = dwTime;
    state.wasteCount = dwWasteCount;
    state.visible = VisiblePileCards;
    state.started = fGameStarted;
    state.score = lScore;
    GetScoreRound(&state.roundBase, &state.roundId);
    int total = 0, foundations = 0;
    for (int i = 0; i < 13; ++i)
    {
        const CardStack &stack = i == 12 ? activepile : SavedRegion(i)->GetCardStack();
        state.count[i] = stack.NumCards();
        total += stack.NumCards();
        if (i >= 8 && i < 12) foundations += stack.NumCards();
        for (int j = 0; j < stack.NumCards(); ++j)
            state.card[i][j] = stack[j].Idx() | (stack[j].FaceUp() ? 0x100 : 0);
    }
    for (int i = 0; i < NUM_ROW_STACKS; ++i)
    {
        int down;
        pRowStack[i]->GetFaceDirection(&down);
        state.faceDown[i] = down;
    }
    if (total == 52 && foundations < 52)
        RegSetValueExW(key, L"SavedGame", 0, REG_BINARY, (const BYTE *)&state, sizeof(state));
    else if (foundations == 52 || (!total && !fGameStarted))
        RegDeleteValueW(key, L"SavedGame");
    // During an in-progress drag the source temporarily lacks its cards.
    // Preserve the last complete record instead of saving/deleting partial state.
    RegCloseKey(key);
    SaveScoreBalance();
}

bool LoadGame(void)
{
    SavedGame state = {};
    HKEY key;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, settingsKey, 0, KEY_QUERY_VALUE, &key) != ERROR_SUCCESS)
        return false;
    DWORD type, size = sizeof(state);
    LONG result = RegQueryValueExW(key, L"SavedGame", NULL, &type, (BYTE *)&state, &size);
    RegCloseKey(key);
    bool legacy = state.version == 1 && size == FIELD_OFFSET(SavedGame, roundBase);
    if (result != ERROR_SUCCESS || type != REG_BINARY ||
        (!legacy && (size != sizeof(state) || state.version != savedVersion)) ||
        (state.rules & ~ruleMask) || state.started > 1 ||
        state.visible > 3 || state.visible > state.count[12] ||
        (state.count[12] && !state.visible) || state.time > 31536000 ||
        state.score < -100000000 || state.score > 100000000)
        return false;

    bool seen[52] = {};
    int total = 0, foundations = 0;
    for (int i = 0; i < 13; ++i)
    {
        if (state.count[i] > 52) return false;
        total += state.count[i];
        if (i >= 8 && i < 12)
        {
            foundations += state.count[i];
            if (state.count[i] > 13) return false;
        }
        for (DWORD j = 0; j < state.count[i]; ++j)
        {
            DWORD value = state.card[i][j];
            DWORD index = value & 0xff;
            if ((value & ~0x13f) || index >= 52 || seen[index]) return false;
            seen[index] = true;
            if (i >= 8 && i < 12 &&
                (!(value & 0x100) || index / 4 + 1 != state.count[i] - j ||
                 index % 4 != (state.card[i][0] & 0xff) % 4))
                return false;
        }
    }
    if (total != 52 || foundations == 52) return false;
    DWORD mode = state.rules & (OPTION_SCORE_STD | OPTION_SCORE_VEGAS);
    if (legacy)
    {
        state.rules |= dwOptions & OPTION_SHOW_TIME;
        state.roundBase = mode == OPTION_SCORE_VEGAS ? state.score + 52 - 5 * foundations : 0;
        GetSystemTimeAsFileTime(&state.roundId);
    }
    if ((!state.roundId.dwLowDateTime && !state.roundId.dwHighDateTime) ||
        state.roundBase < -100000000 || state.roundBase > 100000000) return false;
    if (mode == OPTION_SCORE_VEGAS)
    {
        LONGLONG roundScore = (LONGLONG)state.score - state.roundBase;
        if (roundScore < -52 || roundScore > 208) return false;
    }
    else if (state.score < 0 || state.roundBase != 0) return false;
    for (int i = 0; i < NUM_ROW_STACKS; ++i)
    {
        if (state.faceDown[i] > state.count[i + 1]) return false;
        for (DWORD j = 0; j < state.count[i + 1]; ++j)
            if (!!(state.card[i + 1][j] & 0x100) != (j < state.count[i + 1] - state.faceDown[i]))
                return false;
    }

    // Only mutate the game after validating the complete record.
    StopAutoFinish();
    StopWinAnimation(false);
    for (int i = 0; i < 13; ++i)
    {
        CardStack stack;
        for (int j = (int)state.count[i] - 1; j >= 0; --j)
        {
            Card card(state.card[i][j] & 0xff);
            card.SetFaceUp((state.card[i][j] & 0x100) != 0);
            stack.Push(card);
        }
        if (i == 12) activepile = stack;
        else
        {
            if (i >= 1 && i <= NUM_ROW_STACKS)
                SavedRegion(i)->SetFaceDirection(CS_FACE_DOWNUP, state.faceDown[i - 1]);
            SavedRegion(i)->SetCardStack(stack);
        }
    }
    dwOptions = (dwOptions & ~ruleMask) | state.rules;
    dwTime = state.time;
    dwWasteCount = state.wasteCount;
    VisiblePileCards = state.visible;
    lScore = state.score;
    fGameStarted = state.started != 0;
    RestoreScoreRound(state.roundBase, state.roundId);
    dwPrevMode = GetScoreMode();
    dwWasteTreshold = GetScoreMode() == SCORE_VEGAS ? ((dwOptions & OPTION_THREE_CARDS) ? 2 : 0)
                                                   : ((dwOptions & OPTION_THREE_CARDS) ? 3 : 0);
    pPile->SetOffsets(VisiblePileCards > 1 ? CS_DEFXOFF : 0, VisiblePileCards > 1 ? 1 : 0);
    pPile->SetCardStack(VisiblePileCards > 1 ? activepile.Top(VisiblePileCards) : activepile);
    ClearUndo();
    UpdateStatusBar();
    SolWnd.Redraw();
    if (fGameStarted) SetPlayTimer();
    return true;
}
