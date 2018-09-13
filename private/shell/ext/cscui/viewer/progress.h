#ifndef _INC_CSCVIEW_PROGRESS_H
#define _INC_CSCVIEW_PROGRESS_H

#ifndef _INC_CSCVIEW_OBJTREE_H
#   include "objtree.h"
#endif

#ifndef _INC_CSCVIEW_SUBCLASS_H
#   include "subclass.h"
#endif

class CacheView; // fwd decl.

class ProgressBarSource
{
    public:
        ProgressBarSource(void) {  }
        virtual ~ProgressBarSource(void) { }

        virtual int GetMaxCount(void) const = 0;
        virtual int GetCount(void) const = 0;
};



class ProgressBar; // fwd decl.

#define PBN_100PCT (WM_USER + 5001)

const UINT ID_UPDATE_TIMER = 1234;

class ProgressBar
{
    public:
        ProgressBar(void)
            : m_hwndPB(NULL),
              m_hwndNotify(NULL),
              m_cUpdateIntervalMs(0),
              m_cMaxPos(0),
              m_pSource(NULL),
              m_bKillPending(false),
              m_PBSubclass(*this, ID_UPDATE_TIMER) { DBGTRACE((TEXT("ProgressBar::ProgressBar"))); };

        ~ProgressBar(void);

        bool Create(HWND hwndPB, HWND hwndNotify, int cUpdateIntervalMs);
        bool Create(HWND hwndPB, HWND hwndNotify, const CacheView& view, int cUpdateIntervalMs, const CscObjTree& tree);
        bool Create(HWND hwndPB, HWND hwndNotify, const CacheView& view, int cUpdateIntervalMs, const CscShare& share);
        void Destroy(void)
            { m_PBSubclass.Cancel(); }
        void Reset(void);

    private:
        class Subclass : public WindowSubclass
        {
            public:
                Subclass(ProgressBar& bar,
                         UINT idTimer)
                    : m_bar(bar),
                      m_idTimer(idTimer),
                      m_bTimerIsDead(true) { }

                ~Subclass(void) { }

                void StartTimer(int cUpdateIntervalMs);
                void KillTimer(void);

            private:
                UINT m_idTimer;
                ProgressBar& m_bar;
                bool m_bTimerIsDead;

                virtual LRESULT HandleMessages(HWND, UINT, WPARAM, LPARAM);
                
        } m_PBSubclass;

        class ViewSource : public ProgressBarSource
        {
            public:
                ViewSource(const CacheView& view)
                    : m_view(view) { }

                ~ViewSource(void) { }

                virtual int GetCount(void) const;

                virtual int GetMaxCount(void) const
                    { return 0; }

            private:
                const CacheView& m_view;
        };

        class ShareSource : public ProgressBarSource
        {
            public:
                ShareSource(const CacheView& view, const CscShare& share) 
                    : m_srcView(view), 
                      m_share(share) { }

                ~ShareSource(void) { }

                virtual int GetMaxCount(void) const;

                virtual int GetCount(void) const;

            private:
                const ViewSource m_srcView;
                const CscShare&  m_share;
        };

        class TreeSource : public ProgressBarSource
        {
            public:
                TreeSource(const CacheView& view, const CscObjTree& tree)
                    : m_srcView(view),
                      m_tree(tree) { }

                ~TreeSource(void) { }

                virtual int GetMaxCount(void) const;

                virtual int GetCount(void) const;

            private:
                const ViewSource  m_srcView;
                const CscObjTree& m_tree;
        };

        class NullSource : public ProgressBarSource
        {
            public:
                NullSource(void) { }
                ~NullSource(void) { }

                virtual int GetMaxCount(void) const
                    { return 0; }

                virtual int GetCount(void) const
                    { return 0; }
        };


        HWND               m_hwndPB;            // Progress bar window.
        HWND               m_hwndNotify;        // Notify this wnd.
        int                m_cUpdateIntervalMs; // Update timer interval (ms).
        int                m_cMaxPos;           // Upper end of range.
        bool               m_bKillPending;      // Kill next update?
        ProgressBarSource *m_pSource;           // Virtual source object.

        void Update(void);
        bool CommonCreate(HWND hwndPB, HWND hwndNotify, int cUpdateIntervalMs);

        friend class Subclass;  // for calling Update().
};


#endif // _INC_CSCVIEW_PROGRESS_H

