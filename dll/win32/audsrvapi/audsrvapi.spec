@ stdcall InitStream (ptr long long long long long long long long)
@ stdcall PlayAudio ( ptr);
@ stdcall StopAudio (ptr );
@ stdcall Volume(ptr ptr );
@ stdcall SetVolume(ptr long);
@ stdcall Write(ptr ptr);
@ stdcall SetBalance(ptr long);
@ stdcall GetBalance(ptr ptr);