#include "stdafx.h"

/*
    Get cost of insertion Req1 before Req2
 */
LONGLONG
NTAPI
UniataGetCost(
    PHW_LU_EXTENSION LunExt,
    IN PATA_REQ AtaReq1,
    IN PATA_REQ AtaReq2
    )
{
    BOOLEAN write1;
    BOOLEAN write2;
    UCHAR flags1;
    UCHAR flags2;
    LONGLONG cost;
    // can't insert Req1 before tooooo old Req2
    if(!AtaReq2->ttl)
        return REORDER_COST_TTL;
    // check if reorderable
    flags1 = AtaReq1->Flags;
    flags2 = AtaReq2->Flags;
    if(!((flags2 & flags1) & REQ_FLAG_REORDERABLE_CMD))
        return REORDER_COST_DENIED;
    // if at least one Req is WRITE, target areas
    // can not intersect
    write1 = (flags1 & REQ_FLAG_RW_MASK) == REQ_FLAG_WRITE;
    write2 = (flags2 & REQ_FLAG_RW_MASK) == REQ_FLAG_WRITE;
    cost = AtaReq1->lba+AtaReq1->bcount - AtaReq2->lba;
    if( write1 || write2 ) {
        // check for intersection
        if((AtaReq1->lba < AtaReq2->lba+AtaReq2->bcount) &&
           (AtaReq1->lba+AtaReq1->bcount > AtaReq2->lba)) {
            // Intersection...
            return REORDER_COST_INTERSECT;
        }
    }
    if(cost < 0) {
        cost *= (1-LunExt->SeekBackMCost);
    } else {
        cost *= (LunExt->SeekBackMCost-1);
    }
    if( write1 == write2 ) {
        return cost;
    }
    return (cost * LunExt->RwSwitchMCost) + LunExt->RwSwitchCost;
} // end UniataGetCost()

/*
    Insert command to proper place of command queue
    Perform reorder if necessary
 */
VOID
NTAPI
UniataQueueRequest(
    IN PHW_CHANNEL chan,
    IN PSCSI_REQUEST_BLOCK Srb
    )
{
    PATA_REQ AtaReq = (PATA_REQ)(Srb->SrbExtension);
    PATA_REQ AtaReq1;
    PATA_REQ AtaReq2;

    LONGLONG best_cost;
    LONGLONG new_cost;
    LONGLONG new_cost1;
    LONGLONG new_cost2;
    PATA_REQ BestAtaReq1;

    BOOLEAN use_reorder = chan->UseReorder/*EnableReorder*/;
#ifdef QUEUE_STATISTICS
    BOOLEAN reordered = FALSE;
#endif //QUEUE_STATISTICS

    PHW_LU_EXTENSION LunExt = chan->lun[GET_LDEV(Srb) & 1];
    AtaReq->Srb = Srb;

/*
#ifdef _DEBUG
    if(!LunExt) {
        PrintNtConsole("q: chan = %#x, dev %#x\n", chan, GET_LDEV(Srb));
        int i;
        for(i=0; i<1000; i++) {
            AtapiStallExecution(5*1000);
        }
        return;
    }
#endif //_DEBUG
*/
    // check if queue is empty
    if(LunExt->queue_depth) {
        AtaReq->ttl = (UCHAR)(max(LunExt->queue_depth, MIN_REQ_TTL));

        // init loop
        BestAtaReq1 =
        AtaReq2 = LunExt->last_req;

        if(use_reorder &&
           (Srb->SrbFlags & SRB_FLAGS_QUEUE_ACTION_ENABLE)) {
            switch(Srb->QueueAction) {
            case SRB_ORDERED_QUEUE_TAG_REQUEST:
                use_reorder = FALSE;
#ifdef QUEUE_STATISTICS
                chan->TryReorderTailCount++;
#endif //QUEUE_STATISTICS
                break;
            case SRB_HEAD_OF_QUEUE_TAG_REQUEST:
                BestAtaReq1 = LunExt->first_req;
                best_cost = -REORDER_COST_RESELECT;
#ifdef QUEUE_STATISTICS
                chan->TryReorderHeadCount++;
#endif //QUEUE_STATISTICS
                break;
            case SRB_SIMPLE_TAG_REQUEST:
            default:
                best_cost = UniataGetCost(LunExt, BestAtaReq1, AtaReq);
                use_reorder |= TRUE;
            }
        } else
        if(use_reorder) {
            best_cost = UniataGetCost(LunExt, BestAtaReq1, AtaReq);
        }

        if(use_reorder) {

#ifdef QUEUE_STATISTICS
            chan->TryReorderCount++;
#endif //QUEUE_STATISTICS

            // walk through command queue and find the best
            // place for insertion of the command
            while ((AtaReq1 = AtaReq2->prev_req)) {
                new_cost1 = UniataGetCost(LunExt, AtaReq1, AtaReq);
                new_cost2 = UniataGetCost(LunExt, AtaReq, AtaReq2);

#ifdef QUEUE_STATISTICS
                if(new_cost1 == REORDER_COST_INTERSECT ||
                   new_cost2 == REORDER_COST_INTERSECT)
                    chan->IntersectCount++;
#endif //QUEUE_STATISTICS

                if(new_cost2 > REORDER_COST_RESELECT)
                    break;

                // for now I see nothing bad in RESELECT here
                //ASSERT(new_cost1 <= REORDER_COST_RESELECT);

                new_cost = UniataGetCost(LunExt, AtaReq1, AtaReq2);
                new_cost = new_cost1 + new_cost2 - new_cost;

                // check for Stop Reordering
                if(new_cost > REORDER_COST_RESELECT*3)
                    break;

                if(new_cost < best_cost) {
                    best_cost = new_cost;
                    BestAtaReq1 = AtaReq1;
#ifdef QUEUE_STATISTICS
                    reordered = TRUE;
#endif //QUEUE_STATISTICS
                }
                AtaReq2 = AtaReq1;
            }
#ifdef QUEUE_STATISTICS
            if(reordered)
                chan->ReorderCount++;
#endif //QUEUE_STATISTICS
        }
        // Insert Req between Req2 & Req1
        AtaReq2 = BestAtaReq1->next_req;
        if(AtaReq2) {
            AtaReq2->prev_req = AtaReq;
            AtaReq->next_req = AtaReq2;
        } else {
            AtaReq->next_req = NULL;
            LunExt->last_req = AtaReq;
        }
//        LunExt->last_req->next_req = AtaReq;
        BestAtaReq1->next_req = AtaReq;
//        AtaReq->prev_req = LunExt->last_req;
        AtaReq->prev_req = BestAtaReq1;

        AtaReq1 = AtaReq;
        while((AtaReq1 = AtaReq1->next_req)) {
            //ASSERT(AtaReq1->ttl);
            AtaReq1->ttl--;
        }

    } else {
        // empty queue
        AtaReq->ttl = 0;
        AtaReq->prev_req =
        AtaReq->next_req = NULL;
        LunExt->first_req =
        LunExt->last_req = AtaReq;
    }
    LunExt->queue_depth++;
    chan->queue_depth++;
    chan->DeviceExtension->queue_depth++;
    // check if this is the 1st request in queue
    if(chan->queue_depth == 1) {
        chan->cur_req = LunExt->first_req;
    }

#ifdef QUEUE_STATISTICS
    //ASSERT(LunExt->queue_depth);
    chan->QueueStat[min(MAX_QUEUE_STAT, LunExt->queue_depth)-1]++;
#endif //QUEUE_STATISTICS
/*
    ASSERT(chan->queue_depth ==
            (chan->lun[0]->queue_depth + chan->lun[1]->queue_depth));
    ASSERT(!chan->queue_depth || chan->cur_req);
*/
    return;
} // end UniataQueueRequest()

/*
    Remove request from queue and get next request
 */
VOID
NTAPI
UniataRemoveRequest(
    IN PHW_CHANNEL chan,
    IN PSCSI_REQUEST_BLOCK Srb
    )
{
    if(!Srb)
        return;
    if(!chan)
        return;

    PATA_REQ AtaReq = (PATA_REQ)(Srb->SrbExtension);
    //PHW_DEVICE_EXTENSION deviceExtension = chan->DeviceExtension;

    ULONG cdev = GET_LDEV(Srb) & 1;
    PHW_LU_EXTENSION LunExt = chan->lun[cdev];

    if(!LunExt)
        return;

/*
    ASSERT(chan->queue_depth ==
            (chan->lun[0]->queue_depth + chan->lun[1]->queue_depth));
    ASSERT(!chan->queue_depth || chan->cur_req);
*/
    // check if queue for the device is empty
    if(!LunExt->queue_depth)
        return;

    // check if request is under processing (busy)
    if(!AtaReq->ReqState)
        return;

    // remove reqest from queue
    if(AtaReq->prev_req) {
        AtaReq->prev_req->next_req =
            AtaReq->next_req;
    } else {
        LunExt->first_req = AtaReq->next_req;
    }
    if(AtaReq->next_req) {
        AtaReq->next_req->prev_req =
            AtaReq->prev_req;
    } else {
        LunExt->last_req = AtaReq->prev_req;
    }
    LunExt->queue_depth--;
    chan->queue_depth--;
    chan->DeviceExtension->queue_depth--;
    // set LastWrite flag for Lun
    LunExt->last_write = ((AtaReq->Flags & REQ_FLAG_RW_MASK) == REQ_FLAG_WRITE);

    // get request from longest queue to balance load
    if(chan->lun[0]->queue_depth * (chan->lun[0]->LunSelectWaitCount+1) >
       chan->lun[1]->queue_depth * (chan->lun[1]->LunSelectWaitCount+1)) {
        cdev = 0;
    } else {
        cdev = 1;
    }
/*    // prevent too long wait for actively used device
    if(chan->lun[cdev ^ 1]->queue_depth &&
       chan->lun[cdev ^ 1]->LunSelectWaitCount >= chan->lun[cdev]->queue_depth) {
        cdev = cdev ^ 1;
    }*/
    // get next request for processing
    chan->cur_req = chan->lun[cdev]->first_req;
    chan->cur_cdev = cdev;
    if(!chan->lun[cdev ^ 1]->queue_depth) {
        chan->lun[cdev ^ 1]->LunSelectWaitCount=0;
    } else {
        chan->lun[cdev ^ 1]->LunSelectWaitCount++;
    }
    chan->lun[cdev]->LunSelectWaitCount=0;

//    chan->first_req->prev_req = NULL;
    AtaReq->ReqState = REQ_STATE_NONE;
/*
    ASSERT(chan->queue_depth ==
            (chan->lun[0]->queue_depth + chan->lun[1]->queue_depth));
    ASSERT(!chan->queue_depth || chan->cur_req);
*/
    return;
} // end UniataRemoveRequest()

/*
    Get currently processed request
    (from head of the queue)
 */
PSCSI_REQUEST_BLOCK
NTAPI
UniataGetCurRequest(
    IN PHW_CHANNEL chan
    )
{
//    if(!chan->lun[]->queue_depth) {
    if(!chan || !chan->cur_req) {
        return NULL;
    }

    return chan->cur_req->Srb;
} // end UniataGetCurRequest()

/*
    Get next channel to be serviced
    (used in simplex mode only)
 */
PHW_CHANNEL
NTAPI
UniataGetNextChannel(
    IN PHW_CHANNEL chan
    )
{
    ULONG c=0, _c;
    PHW_DEVICE_EXTENSION deviceExtension;
    ULONG best_c;
    ULONG cost_c;

    deviceExtension = chan->DeviceExtension;

    if(!deviceExtension->simplexOnly) {
        return chan;
    }
    KdPrint2((PRINT_PREFIX "UniataGetNextChannel:\n"));
    best_c = -1;
    cost_c = 0;
    for(_c=0; _c<deviceExtension->NumberChannels; _c++) {
        c = (_c+deviceExtension->FirstChannelToCheck) % deviceExtension->NumberChannels;
        chan = &deviceExtension->chan[c];
        if(chan->queue_depth &&
           chan->queue_depth * (chan->ChannelSelectWaitCount+1) >
           cost_c) {
            best_c = c;
            cost_c = chan->queue_depth * (chan->ChannelSelectWaitCount+1);
        }
    }
    if(best_c == CHAN_NOT_SPECIFIED) {
        KdPrint2((PRINT_PREFIX "  empty queues\n"));
        return NULL;
    }
    deviceExtension->FirstChannelToCheck = c;
    for(_c=0; _c<deviceExtension->NumberChannels; _c++) {
        chan = &deviceExtension->chan[_c];
        if(_c == best_c) {
            chan->ChannelSelectWaitCount = 0;
            continue;
        }
        chan->ChannelSelectWaitCount++;
        if(!chan->queue_depth) {
            chan->ChannelSelectWaitCount = 0;
        } else {
            chan->ChannelSelectWaitCount++;
        }
    }
    KdPrint2((PRINT_PREFIX "  select chan %d\n", best_c));
    return &deviceExtension->chan[best_c];
} // end UniataGetNextChannel()
