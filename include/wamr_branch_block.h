//
// Created by victoryang00 on 5/2/23.
//

#ifndef MVVM_WAMR_BRANCH_BLOCK_H
#define MVVM_WAMR_BRANCH_BLOCK_H
#include "wamr_module_instance.h"
#include "wasm_runtime.h"
#include <memory>
struct WAMRBranchBlock {
    uint32 begin_addr{}; // real code section for native code?WASMBranchBlock
    uint32 target_addr{};
    uint32 frame_sp{};
    uint32 cell_num{};

    void dump_impl(WASMBranchBlock *env);
    void restore_impl(WASMBranchBlock *env) const;
};

template <SerializerTrait<WASMBranchBlock *> T> void dump(T t, WASMBranchBlock *env) { t->dump_impl(env); }
template <SerializerTrait<WASMBranchBlock *> T> void restore(T t, WASMBranchBlock *env) { t->restore_impl(env); }
#endif // MVVM_WAMR_BRANCH_BLOCK_H
