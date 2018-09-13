#
# NEC98 product requires the US clock.avi renamed.
#

$(O)\winclock.avi: ..\clock.avi
    copy ..\clock.avi $@
