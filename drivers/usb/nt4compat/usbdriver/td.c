/**
 * td.c - USB driver stack project for Windows NT 4.0
 *
 * Copyright (c) 2002-2004 Zhiming  mypublic99@yahoo.com
 *
 * This program/include file is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program/include file is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program (in the main directory of the distribution, the file
 * COPYING); if not, write to the Free Software Foundation,Inc., 59 Temple
 * Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "usbdriver.h"

#define UHCI_MIN_TD_POOLS 4

BOOLEAN free_td_to_pool(PUHCI_TD_POOL ptd_pool, PUHCI_TD ptd);     //add tds till pnext == NULL


PUHCI_QH alloc_qh(PUHCI_QH_POOL pqh_pool);      //null if failed

BOOLEAN
init_td_pool(PUHCI_TD_POOL ptd_pool)
{
    int i, pages;
    PTD_EXTENSION ptde;

    if (ptd_pool == NULL)
        return FALSE;

    if (ptd_pool->padapter == NULL)
        return FALSE;

    pages = sizeof(UHCI_TD) * UHCI_MAX_POOL_TDS / PAGE_SIZE;
    RtlZeroMemory(ptd_pool->td_array, sizeof(ptd_pool->td_array));
    RtlZeroMemory(ptd_pool->logic_addr, sizeof(ptd_pool->logic_addr));

    for(i = 0; i < pages; i++)
    {
        ptd_pool->td_array[i] =
            HalAllocateCommonBuffer(ptd_pool->padapter, PAGE_SIZE, &ptd_pool->logic_addr[i], FALSE);
        if (ptd_pool->td_array[i] == NULL)
            goto failed;
    }

    ptd_pool->tde_array = (PTD_EXTENSION) usb_alloc_mem(NonPagedPool,
                                                        sizeof(TD_EXTENSION) * UHCI_MAX_POOL_TDS);

    if (ptd_pool->tde_array == NULL)
        goto failed;

    for(i = 0; i < pages; i++)
    {
        RtlZeroMemory(ptd_pool->td_array[i], PAGE_SIZE);
    }

    RtlZeroMemory(ptd_pool->tde_array, sizeof(TD_EXTENSION) * UHCI_MAX_POOL_TDS);

    ptde = ptd_pool->tde_array;
    ptd_pool->free_count = 0;
    ptd_pool->total_count = UHCI_MAX_POOL_TDS;
    InitializeListHead(&ptd_pool->free_que);

    for(i = 0; i < UHCI_MAX_POOL_TDS; i++)
    {
        //link tde and the td one by one, fixed since this init
        ptd_pool->td_array[i >> 7][i & 0x7f].ptde = &ptde[i];
        ptde[i].ptd = &ptd_pool->td_array[i >> 7][i & 0x7f];
        ptde[i].flags = UHCI_ITEM_FLAG_TD;
        ptd_pool->td_array[i >> 7][i & 0x7f].phy_addr =
            ptd_pool->logic_addr[i >> 7].LowPart + (i & 0x7f) * sizeof(UHCI_TD);
        ptd_pool->td_array[i >> 7][i & 0x7f].pool = ptd_pool;
        ptd_pool->td_array[i >> 7][i & 0x7f].purb = NULL;
        free_td_to_pool(ptd_pool, &ptd_pool->td_array[i >> 7][i & 0x7f]);

    }
    return TRUE;

failed:
    for(i = 0; i < pages; i++)
    {
        if (ptd_pool->td_array[i])
        {
            HalFreeCommonBuffer(ptd_pool->padapter,
                                PAGE_SIZE, ptd_pool->logic_addr[i], ptd_pool->td_array[i], FALSE);
            ptd_pool->td_array[i] = NULL;
            ptd_pool->logic_addr[i].QuadPart = 0;
        }
    }

    if (ptd_pool->tde_array)
        usb_free_mem(ptd_pool->tde_array);

    uhci_dbg_print(DBGLVL_MAXIMUM, ("init_td_pool(): failed to init the td pool\n"));
    TRAP();

    ptd_pool->free_count = ptd_pool->total_count = 0;
    return FALSE;
}

//add tds till pnext == NULL
BOOLEAN
free_td_to_pool(PUHCI_TD_POOL ptd_pool, PUHCI_TD ptd)
{
    if (ptd_pool == NULL || ptd == NULL)
    {
        return FALSE;
    }

    ptd->link = ptd->status = ptd->info = ptd->buffer = 0;
    ptd->purb = NULL;
    ptd_pool->free_count++;

    InsertTailList(&ptd_pool->free_que, &ptd->ptde->vert_link);

    return TRUE;

}

// qh routines

//null if failed
PUHCI_TD
alloc_td_from_pool(PUHCI_TD_POOL ptd_pool)
{
    PTD_EXTENSION ptde;
    PLIST_ENTRY temp;

    if (ptd_pool == NULL)
        return FALSE;

    if (IsListEmpty(&ptd_pool->free_que))
        return FALSE;

    temp = RemoveHeadList(&ptd_pool->free_que);

    if (temp == NULL)
        return FALSE;

    ptde = struct_ptr(temp, TD_EXTENSION, vert_link);

    ptd_pool->free_count--;

    InitializeListHead(&ptde->vert_link);
    InitializeListHead(&ptde->hori_link);

    return ptde->ptd;

}

//test whether the pool is all free
BOOLEAN
is_pool_free(PUHCI_TD_POOL pool)
{
    if (pool == NULL)
        return FALSE;

    if (pool->free_count == pool->total_count)
        return TRUE;

    return FALSE;
}

BOOLEAN
is_pool_empty(PUHCI_TD_POOL pool)
{
    if (pool == NULL)
        return FALSE;

    return (BOOLEAN) (pool->free_count == 0);
}

BOOLEAN
destroy_td_pool(PUHCI_TD_POOL ptd_pool)
{
    int i, pages;
    PADAPTER_OBJECT padapter;   //we need this garbage for allocation

    padapter = ptd_pool->padapter;

    pages = sizeof(UHCI_TD) * UHCI_MAX_POOL_TDS / PAGE_SIZE;
    if (ptd_pool && ptd_pool->padapter)
    {
        usb_free_mem(ptd_pool->tde_array);
        ptd_pool->tde_array = NULL;
        for(i = 0; i < pages; i++)
        {
            if (ptd_pool->td_array[i])
            {
                HalFreeCommonBuffer(ptd_pool->padapter,
                                    PAGE_SIZE, ptd_pool->logic_addr[i], ptd_pool->td_array[i], FALSE);
                ptd_pool->td_array[i] = NULL;
                ptd_pool->logic_addr[i].QuadPart = 0;
            }
        }
        RtlZeroMemory(ptd_pool, sizeof(UHCI_TD_POOL));
        ptd_pool->padapter = padapter;
        ptd_pool->free_count = ptd_pool->total_count = 0;
    }
    else
        return FALSE;

    return TRUE;
}

BOOLEAN
init_td_pool_list(PUHCI_TD_POOL_LIST pool_list, PADAPTER_OBJECT padapter)
{
    int i;
    RtlZeroMemory(pool_list, sizeof(UHCI_TD_POOL_LIST));
    InitializeListHead(&pool_list->busy_pools);
    InitializeListHead(&pool_list->free_pools);

    pool_list->free_count = UHCI_MAX_TD_POOLS;
    pool_list->free_tds = 0;

    for(i = 0; i < UHCI_MAX_TD_POOLS; i++)
    {
        pool_list->pool_array[i].padapter = padapter;
        InsertTailList(&pool_list->free_pools, &pool_list->pool_array[i].pool_link);
    }

    KeInitializeSpinLock(&pool_list->pool_lock);
    return expand_pool_list(pool_list, UHCI_MIN_TD_POOLS);
}

BOOLEAN
destroy_td_pool_list(PUHCI_TD_POOL_LIST pool_list)
{
    PUHCI_TD_POOL pool;
    while (IsListEmpty(&pool_list->busy_pools) == FALSE)
    {
        pool = (PUHCI_TD_POOL) RemoveHeadList(&pool_list->busy_pools);
        destroy_td_pool(pool);
    }

    RtlZeroMemory(pool_list, sizeof(UHCI_TD_POOL_LIST));
    return TRUE;
}

BOOLEAN
expand_pool_list(PUHCI_TD_POOL_LIST pool_list, LONG pool_count) //private
{
    PUHCI_TD_POOL pool;
    int i;

    if (IsListEmpty(&pool_list->free_pools) == TRUE)
        return FALSE;

    if (pool_list->free_count < pool_count)
        return FALSE;

    for(i = 0; i < pool_count; i++)
    {
        pool = (PUHCI_TD_POOL) RemoveHeadList(&pool_list->free_pools);

        if (init_td_pool(pool) == FALSE)
        {
            //reverse the allocation
            InsertHeadList(&pool_list->free_pools, &pool->pool_link);
            // collect_garbage( pool_list );
            return FALSE;
        }

        InsertTailList(&pool_list->busy_pools, &pool->pool_link);
        pool_list->free_tds += UHCI_MAX_POOL_TDS;
        pool_list->free_count--;
    }
    return TRUE;
}

BOOLEAN
collect_garbage(PUHCI_TD_POOL_LIST pool_list)
{
    PLIST_ENTRY prev, next;

    // no garbage
    if (pool_list->free_count >= UHCI_MAX_TD_POOLS - UHCI_MIN_TD_POOLS)
        return TRUE;

    ListFirstPrev(&pool_list->busy_pools, prev);
    ListNext(&pool_list->busy_pools, prev, next);

    while (next && next != &pool_list->busy_pools)
    {
        if (is_pool_free((PUHCI_TD_POOL) next))
        {
            RemoveEntryList(next);
            destroy_td_pool((PUHCI_TD_POOL) next);
            InsertTailList(&pool_list->free_pools, next);
            pool_list->free_count++;
            pool_list->free_tds -= UHCI_MAX_POOL_TDS;
            ListNext(&pool_list->busy_pools, prev, next);
            if (pool_list->free_count >= UHCI_MAX_TD_POOLS - UHCI_MIN_TD_POOLS)
                break;
        }
        else
        {
            prev = next;
            ListNext(&pool_list->busy_pools, prev, next);
        }
    }
    return TRUE;

}

//private
LONG
get_num_free_tds(PUHCI_TD_POOL_LIST pool_list)
{
    return pool_list->free_tds;
}

//private
LONG
get_max_free_tds(PUHCI_TD_POOL_LIST pool_list)
{
    return pool_list->free_tds + pool_list->free_count * UHCI_MAX_POOL_TDS;
}

//add tds till pnext == NULL
BOOLEAN
free_td(PUHCI_TD_POOL_LIST pool_list, PUHCI_TD ptd)
{
    if (pool_list == NULL || ptd == NULL)
        return FALSE;

    if (free_td_to_pool(ptd->pool, ptd) == FALSE)
        return FALSE;

    pool_list->free_tds++;

    if (is_pool_free(ptd->pool))
    {
        collect_garbage(pool_list);
    }
    return TRUE;
}

//null if failed
PUHCI_TD
alloc_td(PUHCI_TD_POOL_LIST pool_list)
{
    PLIST_ENTRY prev, next;
    PUHCI_TD new_td;

    if (pool_list == NULL)
        return NULL;

    if (pool_list->free_tds == 0)
    {
        if (expand_pool_list(pool_list, 1) == FALSE)
            return NULL;
    }

    ListFirst(&pool_list->busy_pools, prev);

    while (prev && prev != &pool_list->busy_pools)
    {
        if (is_pool_empty((PUHCI_TD_POOL) prev) == FALSE)
        {
            new_td = alloc_td_from_pool((PUHCI_TD_POOL) prev);

            if (new_td == NULL)
                TRAP();

            pool_list->free_tds--;

            return new_td;
        }

        ListNext(&pool_list->busy_pools, prev, next);
        prev = next;
    }

    return NULL;
}

PUHCI_TD
alloc_tds(PUHCI_TD_POOL_LIST pool_list, LONG count)
{
    //return value is a list of tds, vert_link chain.

    LONG i;
    PUHCI_TD ptd, pnext;

    if (pool_list == NULL || count <= 0)
        return NULL;

    if (count >= get_max_free_tds(pool_list))
        return NULL;

    ptd = alloc_td(pool_list);

    for(i = 1; i < count; i++)
    {
        pnext = alloc_td(pool_list);

        if (pnext)
        {
            InsertTailList(&ptd->ptde->vert_link, &pnext->ptde->vert_link);
        }
        else
            TRAP();
    }

    uhci_dbg_print(DBGLVL_MEDIUM, ("alloc_tds(): td pool-list free_tds=0x%x, free pools=0x%x\n",
                                   pool_list->free_tds, pool_list->free_count));

    return ptd;

}

VOID
free_tds(PUHCI_TD_POOL_LIST pool_list, PUHCI_TD ptd)
{
    PUHCI_TD ptofree;
    PLIST_ENTRY pthis;

    if (pool_list == NULL || ptd == NULL)
        return;

    while (IsListEmpty(&ptd->ptde->vert_link) == FALSE)
    {
        pthis = RemoveHeadList(&ptd->ptde->vert_link);
        ptofree = ((PTD_EXTENSION) pthis)->ptd;
        free_td(pool_list, ptofree);
    }

    free_td(pool_list, ptd);
    return;
}



BOOLEAN
can_transfer(PUHCI_TD_POOL_LIST pool_list, LONG td_count)
{
    if (td_count > get_max_free_tds(pool_list))
        return FALSE;

    return TRUE;
}

VOID
lock_td_pool(PUHCI_TD_POOL_LIST pool_list, BOOLEAN at_dpc)
{
    //if( !at_dpc )
    //        KeAcquireSpinLock( &pool_list->pool_lock );
    //else
    //    KeAcquireSpinLockAtDpcLevel( &pool_list->pool_lock );
}

VOID
unlock_td_pool(PUHCI_TD_POOL_LIST pool_list, BOOLEAN at_dpc)
{
    //if( !at_dpc )
    //    KeReleaseSpinLock( &pool_list->pool_lock );
    //else
    //        KeReleaseSpinLockFromDpcLevel( &pool_list->pool_lock );
}

BOOLEAN
init_qh_pool(PUHCI_QH_POOL pqh_pool, PADAPTER_OBJECT padapter)
{
    PQH_EXTENSION pqhe;
    LONG i;

    if (pqh_pool == NULL || padapter == NULL)
        return FALSE;

    pqh_pool->padapter = padapter;

    pqh_pool->qhe_array = (PQH_EXTENSION) usb_alloc_mem(NonPagedPool,
                                                        sizeof(QH_EXTENSION) * UHCI_MAX_POOL_QHS);

    if (pqh_pool->qhe_array == NULL)
        return FALSE;

    pqh_pool->qh_array =
        (PUHCI_QH) HalAllocateCommonBuffer(padapter,
                                           sizeof(UHCI_QH) * UHCI_MAX_POOL_QHS, &pqh_pool->logic_addr, FALSE);

    if (pqh_pool->qh_array == NULL)
    {
        usb_free_mem(pqh_pool->qhe_array);
        pqh_pool->qhe_array = NULL;
        return FALSE;
    }

    pqhe = pqh_pool->qhe_array;

    pqh_pool->free_count = 0;
    pqh_pool->total_count = UHCI_MAX_POOL_TDS;

    KeInitializeSpinLock(&pqh_pool->pool_lock);
    InitializeListHead(&pqh_pool->free_que);


    for(i = 0; i < UHCI_MAX_POOL_QHS; i++)
    {
        pqh_pool->qh_array[i].pqhe = &pqhe[i];
        pqhe[i].pqh = &pqh_pool->qh_array[i];

        pqh_pool->qh_array[i].phy_addr = (pqh_pool->logic_addr.LowPart + (sizeof(UHCI_QH) * i)) | UHCI_PTR_QH;
        //pqh_pool->qh_array[i].reserved = 0;

        //always breadth first
        pqhe[i].flags = UHCI_ITEM_FLAG_QH;

        free_qh(pqh_pool, &pqh_pool->qh_array[i]);

    }
    return TRUE;

}

//add qhs till pnext == NULL
BOOLEAN
free_qh(PUHCI_QH_POOL pqh_pool, PUHCI_QH pqh)
{
    if (pqh_pool == NULL || pqh == NULL)
        return FALSE;

    pqh->link = pqh->element = 0;
    pqh->pqhe->purb = NULL;
    InsertTailList(&pqh_pool->free_que, &pqh->pqhe->vert_link);
    pqh_pool->free_count++;

    return TRUE;
}

//null if failed
PUHCI_QH
alloc_qh(PUHCI_QH_POOL pqh_pool)
{
    PQH_EXTENSION pqhe;

    if (pqh_pool == NULL)
        return FALSE;

    if (IsListEmpty(&pqh_pool->free_que))
        return FALSE;

    pqhe = (PQH_EXTENSION) RemoveHeadList(&pqh_pool->free_que);

    if (pqhe)
    {
        InitializeListHead(&pqhe->hori_link);
        InitializeListHead(&pqhe->vert_link);
        return pqhe->pqh;
    }
    return NULL;

}

BOOLEAN
destroy_qh_pool(PUHCI_QH_POOL pqh_pool)
{
    if (pqh_pool)
    {
        usb_free_mem(pqh_pool->qhe_array);

        HalFreeCommonBuffer(pqh_pool->padapter,
                            sizeof(UHCI_QH) * UHCI_MAX_POOL_QHS,
                            pqh_pool->logic_addr, pqh_pool->qh_array, FALSE);

        RtlZeroMemory(pqh_pool, sizeof(UHCI_QH_POOL));

    }
    else
        return FALSE;

    return TRUE;
}

VOID
lock_qh_pool(PUHCI_QH_POOL pool, BOOLEAN at_dpc)
{
    //if( !at_dpc )
    //        KeAcquireSpinLock( &pool->pool_lock );
    //else
    //        KeAcquireSpinLockAtDpcLevel( &pool->pool_lock );
}

VOID
unlock_qh_pool(PUHCI_QH_POOL pool, BOOLEAN at_dpc)
{
    //if( !at_dpc )
    //        KeReleaseSpinLock( &pool->pool_lock );
    //else
    //        KeReleaseSpinLockFromDpcLevel( &pool->pool_lock );
}
