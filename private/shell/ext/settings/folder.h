#ifndef __SETTINGS_FOLDER_H
#define __SETTINGS_FOLDER_H

class SettingsFolder
{
    public:
        SettingsFolder(VOID);
        virtual ~SettingsFolder(VOID);

        INT GetTopicCount(VOID)
            { return m_lstTopics.Count(); }

        PSETTINGS_FOLDER_TOPIC GetTopic(INT iTopic);

        BOOL SetCurrentTopicIndex(INT iTopic);

        INT GetCurrentTopicIndex(VOID) const
            { return m_iCurrentTopic; }

        PSETTINGS_FOLDER_TOPIC GetCurrentTopic(VOID)
            { return GetTopic(m_iCurrentTopic); }

        BOOL Initialize(VOID)
            { return SUCCEEDED(LoadTopics()); }

    private:
        PointerList m_lstTopics;
        INT         m_iCurrentTopic;     // Index of current in m_lstTopics.

        HRESULT LoadTopics(VOID) throw(OutOfMemory);
        HRESULT LoadExtensions(PointerList& list) throw(OutOfMemory);
        HRESULT LoadExtensionCategories(PSETTINGS_FOLDER_EXT pExt) throw(OutOfMemory);
        HRESULT LoadExtensionTopics(PSETTINGS_FOLDER_EXT pExt) throw(OutOfMemory);
        HRESULT AddTopic(PSETTINGS_FOLDER_TOPIC pITopic) throw(OutOfMemory);
};


#endif // __SETTINGS_FOLDER_H

