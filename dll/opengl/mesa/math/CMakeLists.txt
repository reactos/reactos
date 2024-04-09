
list(APPEND SOURCE
    m_debug_clip.c
    m_debug_norm.c
    m_debug_xform.c
    m_eval.c
    m_matrix.c
    m_translate.c
    m_vector.c
    m_xform.c
    precomp.h)

add_library(mesa_math STATIC ${SOURCE})
add_dependencies(mesa_math xdk)
add_pch(mesa_math precomp.h SOURCE)
