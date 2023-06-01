//
// Created by victoryang00 on 5/19/23.
//

#include "wamr_interp_frame.h"
#include "wamr.h"
extern WAMRInstance *wamr;
void WAMRInterpFrame::dump(WASMInterpFrame *env) {
    if (env->function) {
        wamr->set_func(env->function->u.func);

        if (env->ip)
            ip = env->ip - env->function->u.func->code; // here we need to get the offset from the code start.

        lp = ((uint8 *)env->lp) - (uint8 *)wamr->get_exec_env()->wasm_stack.s.bottom; // offset to the wasm_stack_top
        LOGV(DEBUG) << "lp" << env->lp[0];
        if (env->sp) {
            sp = reinterpret_cast<uint8 *>(env->sp) -
                 ((uint8 *)wamr->get_exec_env()->wasm_stack.s.bottom); // offset to the wasm_stack_top
        }
        auto csp_size = (env->csp - env->csp_bottom);
        for (int i = 0; i < csp_size; i++) {
            auto *local_csp = new WAMRBranchBlock();
            ::dump(local_csp, env->csp - i);
            csp.emplace_back(local_csp);
        }
        if (env->function)
            ::dump(&function, env->function);
    }
}
void WAMRInterpFrame::restore(WASMInterpFrame *env) {
    env->function = reinterpret_cast<WASMFunctionInstance *>(malloc(sizeof(WASMFunctionInstance)));
    ::restore(&function, env->function);
    if (env->function)
        wamr->set_func(env->function->u.func);
    if (ip)
        env->ip = env->function->u.func->code + ip;
    if (sp)
        env->sp = reinterpret_cast<uint32 *>((uint8 *)wamr->get_exec_env()->wasm_stack.s.bottom + sp);

    if (lp) {
        memcpy(env->lp, reinterpret_cast<uint32 *>((uint8 *)wamr->get_exec_env()->wasm_stack.s.bottom + lp),
               sizeof(uint32));
        env->lp_bak = reinterpret_cast<uint32 *>((uint8 *)wamr->get_exec_env()->wasm_stack.s.bottom + lp);
        LOGV(DEBUG) << "lp" << env->lp[0];
        LOGV(DEBUG) << "lp" << env->lp;
    }
    int i = 0;
    env->csp_bottom = static_cast<WASMBranchBlock *>(malloc(sizeof(WASMBranchBlock) * csp.size()));
    std::reverse(csp.begin(), csp.end());
    for (auto &&csp_item : csp) {
        ::restore(csp_item.get(), env->csp_bottom + i);
        i++;
    }
    env->sp_bottom = env->lp_bak + env->function->param_cell_num + env->function->local_cell_num;
    env->csp_boundary = env->csp_bottom + env->function->u.func->max_block_num;
    env->sp_boundary = env->sp_bottom + env->function->u.func->max_stack_cell_num;
    env->csp = env->csp_bottom + csp.size() - 1;
}
