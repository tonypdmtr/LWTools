cmake_minimum_required(VERSION 3.0)
project(LWTools)

if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    set(CMAKE_C_FLAGS "/W3 /WX /wd4244 /wd5999 /wd6011 /wd4267")
endif()

include_directories(lwlib common)

add_library(lwlib
    lwlib/lw_alloc.c
    lwlib/lw_cmdline.c
    lwlib/lw_error.c
    lwlib/lw_expr.c
    lwlib/lw_free.c
    lwlib/lw_realloc.c
    lwlib/lw_stack.c
    lwlib/lw_string.c
    lwlib/lw_stringlist.c
    ${LWLIB_WIN}
)

add_executable(lwasm
    lwasm/audit.c
    lwasm/cycle.c
    lwasm/cmt.c
    lwasm/debug.c
    lwasm/input.c
    lwasm/insn_bitbit.c
    lwasm/insn_gen.c
    lwasm/insn_indexed.c
    lwasm/insn_inh.c
    lwasm/insn_logicmem.c
    lwasm/insn_rel.c
    lwasm/insn_rlist.c
    lwasm/insn_rtor.c
    lwasm/insn_tfm.c
    lwasm/instab.c
    lwasm/list.c
    lwasm/lwasm.c
    lwasm/macro.c
    lwasm/main.c
    lwasm/os9.c
    lwasm/output.c
    lwasm/pass1.c
    lwasm/pass2.c
    lwasm/pass3.c
    lwasm/pass4.c
    lwasm/pass5.c
    lwasm/pass6.c
    lwasm/pass7.c
    lwasm/pragma.c
    lwasm/pseudo.c
    lwasm/section.c
    lwasm/struct.c
    lwasm/symbol.c
    lwasm/symdump.c
    lwasm/unicorns.c
)
target_link_libraries (lwasm lwlib)

add_executable(lwlink
    lwlink/expr.c
    lwlink/link.c
    lwlink/lwlink.c
    lwlink/main.c
    lwlink/map.c
    lwlink/output.c
    lwlink/readfiles.c
    lwlink/script.c
)
target_link_libraries(lwlink lwlib)

add_executable(lwar
    lwar/add.c
    lwar/extract.c
    lwar/list.c
    lwar/lwar.c
    lwar/main.c
    lwar/remove.c
    lwar/replace.c
)
target_link_libraries(lwar lwlib)

add_executable(lwobjdump
    lwlink/objdump.c
)
target_link_libraries(lwobjdump lwlib)
