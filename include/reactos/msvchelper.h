
//
#ifdef _M_IX86
void __stdcall wined3d_mutex_lock(void);
void __stdcall wined3d_mutex_unlock(void);
void* __stdcall WineDirect3DCreate(unsigned int dxVersion,void *parent);

void __inline wined3d_mutex_lock_inline(void)
{
    wined3d_mutex_lock();
}

void __inline wined3d_mutex_unlock_inline(void)
{
    wined3d_mutex_unlock();
}

__inline
struct IWineD3D* WineDirect3DCreate_inline(unsigned int dxVersion,void *parent)
{
    return WineDirect3DCreate(dxVersion, parent);
}

#define wined3d_mutex_lock wined3d_mutex_lock_inline
#define wined3d_mutex_unlock wined3d_mutex_unlock_inline
#define WineDirect3DCreate WineDirect3DCreate_inline

#endif /* _M_IX86 */




