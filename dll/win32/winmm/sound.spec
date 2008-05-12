1  pascal -ret16 OpenSound() OpenSound16
2  pascal -ret16 CloseSound() CloseSound16
3  pascal -ret16 SetVoiceQueueSize(word word) SetVoiceQueueSize16
4  pascal -ret16 SetVoiceNote(word word word word) SetVoiceNote16
5  pascal -ret16 SetVoiceAccent(word word word word word) SetVoiceAccent16
6  pascal -ret16 SetVoiceEnvelope(word word word) SetVoiceEnvelope16
7  pascal -ret16 SetSoundNoise(word word) SetSoundNoise16
8  pascal -ret16 SetVoiceSound(word long word) SetVoiceSound16
9  pascal -ret16 StartSound() StartSound16
10 pascal -ret16 StopSound() StopSound16
11 pascal -ret16 WaitSoundState(word) WaitSoundState16
12 pascal -ret16 SyncAllVoices() SyncAllVoices16
13 pascal -ret16 CountVoiceNotes(word) CountVoiceNotes16
14 pascal   GetThresholdEvent() GetThresholdEvent16
15 pascal -ret16 GetThresholdStatus() GetThresholdStatus16
16 pascal -ret16 SetVoiceThreshold(word word) SetVoiceThreshold16
17 pascal -ret16 DoBeep() DoBeep16
18 stub MYOPENSOUND # W1.1, W2.0
