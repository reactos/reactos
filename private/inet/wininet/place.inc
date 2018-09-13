#
#  Places all of the internet binaries into the inetsrv tree
#

# enable building legoizable binaries 
NTLEGO=1


!ifdef _NT386TREE
_NT386TREE=$(_NT386TREE)
!endif

!ifdef _NTMIPSTREE
_NTMIPSTREE=$(_NTMIPSTREE)
!endif

!ifdef _NTALPHATREE
_NTALPHATREE=$(_NTALPHATREE)
!endif

!ifdef _NTPPCTREE
_NTPPCTREE=$(_NTPPCTREE)
!endif
