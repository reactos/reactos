typedef unsigned char err_t;
typedef unsigned int u32_t;
typedef unsigned short u16_t;
typedef unsigned char u8_t;
typedef void sys_sem_t;
typedef void sys_mutex_t;
typedef size_t mem_size_t;
typedef size_t memp_t;
struct pbuf;
struct netif;

void* mem_malloc(mem_size_t size)
{
  __coverity_alloc__(size);
}
void mem_free(void* mem)
{
  __coverity_free__(mem);
}

void* memp_malloc(memp_t type)
{
  __coverity_alloc_nosize__();  
}
void memp_free(memp_t type, void* mem)
{
  __coverity_free__(mem);  
}

void sys_mutex_lock(sys_mutex_t* mutex)
{
  __coverity_exclusive_lock_acquire__(mutex);
}
void sys_mutex_unlock(sys_mutex_t* mutex)
{
  __coverity_exclusive_lock_release__(mutex);
}

u32_t sys_arch_sem_wait(sys_sem_t *sem, u32_t timeout)
{
  __coverity_recursive_lock_acquire__(sem);
}
void sys_sem_signal(sys_sem_t *sem)
{
  __coverity_recursive_lock_release__(sem);
}

err_t ethernet_input(struct pbuf *p, struct netif *inp)
{
  __coverity_tainted_string_sink_content__(p); 
}
err_t tcpip_input(struct pbuf *p, struct netif *inp)
{
  __coverity_tainted_string_sink_content__(p); 
}
err_t ip_input(struct pbuf *p, struct netif *inp)
{
  __coverity_tainted_string_sink_content__(p); 
}
err_t ip4_input(struct pbuf *p, struct netif *inp)
{
  __coverity_tainted_string_sink_content__(p); 
}
err_t ip6_input(struct pbuf *p, struct netif *inp)
{
  __coverity_tainted_string_sink_content__(p); 
}

err_t pbuf_take(struct pbuf *buf, const void *dataptr, u16_t len)
{
  __coverity_tainted_string_argument__(buf);
  __coverity_tainted_data_argument__(buf);
}
err_t pbuf_take_at(struct pbuf *buf, const void *dataptr, u16_t len, u16_t offset)
{
  __coverity_tainted_string_argument__(buf);
  __coverity_tainted_data_argument__(buf);
}
err_t pbuf_copy(struct pbuf *p_to, struct pbuf *p_from)
{
  __coverity_tainted_data_transitive__(p_to, p_from);
}
u16_t pbuf_copy_partial(struct pbuf *p, void *dataptr, u16_t len, u16_t offset)
{
  __coverity_tainted_string_argument__(dataptr);
  __coverity_tainted_data_argument__(dataptr);
}
u8_t pbuf_get_at(struct pbuf* p, u16_t offset)
{
  __coverity_tainted_data_return__();
}

void abort(void)
{
  __coverity_panic__();
}

int check_path(char* path, size_t size)
{
  if (size) {
    __coverity_tainted_data_sanitize__(path);
    return 1;
  } else {
    return 0;
  }
}
