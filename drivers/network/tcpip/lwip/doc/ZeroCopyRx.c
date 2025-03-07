typedef struct my_custom_pbuf
{
   struct pbuf_custom p;
   void* dma_descriptor;
} my_custom_pbuf_t;

LWIP_MEMPOOL_DECLARE(RX_POOL, 10, sizeof(my_custom_pbuf_t), "Zero-copy RX PBUF pool");

void my_pbuf_free_custom(void* p)
{
  SYS_ARCH_DECL_PROTECT(old_level);

  my_custom_pbuf_t* my_puf = (my_custom_pbuf_t*)p;

  // invalidate data cache here - lwIP and/or application may have written into buffer!
  // (invalidate is faster than flushing, and no one needs the correct data in the buffer)
  invalidate_cpu_cache(p->payload, p->tot_len);

  SYS_ARCH_PROTECT(old_level);
  free_rx_dma_descriptor(my_pbuf->dma_descriptor);
  LWIP_MEMPOOL_FREE(RX_POOL, my_pbuf);
  SYS_ARCH_UNPROTECT(old_level);
}

void eth_rx_irq()
{
  dma_descriptor*   dma_desc = get_RX_DMA_descriptor_from_ethernet();
  my_custom_pbuf_t* my_pbuf  = (my_custom_pbuf_t*)LWIP_MEMPOOL_ALLOC(RX_POOL);

  my_pbuf->p.custom_free_function = my_pbuf_free_custom;
  my_pbuf->dma_descriptor         = dma_desc;

  invalidate_cpu_cache(dma_desc->rx_data, dma_desc->rx_length);
  
  struct pbuf* p = pbuf_alloced_custom(PBUF_RAW,
     dma_desc->rx_length,
     PBUF_REF,
     &my_pbuf->p,
     dma_desc->rx_data,
     dma_desc->max_buffer_size);

  if(netif->input(p, netif) != ERR_OK) {
    pbuf_free(p);
  }
}
