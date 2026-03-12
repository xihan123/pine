//
// Created by canyie on 2020/8/28.
// x86_64 support added for NDK 29
//

#include "x86_64.h"
#include "../../utils/memory.h"
#include "../../utils/scoped_memory_access_protection.h"

using namespace pine;

// x86_64 bridge jump trampoline
// Note: x86_64 uses different calling convention (System V AMD64 ABI)
// First 6 integer arguments: rdi, rsi, rdx, rcx, r8, r9
// Return value: rax
static const unsigned char bridge_jump_trampoline[] = {
        // Save rdi (first arg - ArtMethod* method)
        0x57,                                   // push rdi
        // Compare method (in rdi) with target method
        0x48, 0xBF, 0x00, 0x00, 0x00, 0x00,    // mov rdi, 0x0000000000000000 (target method address)
        0x00, 0x00, 0x00, 0x00,
        0x48, 0x39, 0xF8,                       // cmp rax, rdi
        0x75, 0x2A,                             // jne jump_to_original
        // Setup for bridge call
        0x48, 0xBF, 0x00, 0x00, 0x00, 0x00,    // mov rdi, 0x0000000000000000 (extras address)
        0x00, 0x00, 0x00, 0x00,
        0x48, 0x89, 0x77, 0x08,                // mov [rdi+8], rsi (save arg2)
        0x48, 0x89, 0x57, 0x10,                // mov [rdi+16], rdx (save arg3)
        0x48, 0x89, 0x4F, 0x18,                // mov [rdi+24], rcx (save arg4)
        0x48, 0x89, 0xC6,                      // mov rsi, rdi (second arg = extras)
        0x48, 0x89, 0xE2,                      // mov rdx, rsp (third arg = sp)
        0x5F,                                  // pop rdi (restore rdi)
        0x48, 0xB8, 0x00, 0x00, 0x00, 0x00,    // mov rax, 0x0000000000000000 (bridge method address)
        0x00, 0x00, 0x00, 0x00,
        0x48, 0xB9, 0x00, 0x00, 0x00, 0x00,    // mov rcx, 0x0000000000000000 (bridge entry)
        0x00, 0x00, 0x00, 0x00,
        0xFF, 0xE1,                            // jmp rcx
        // jump_to_original:
        0x5F,                                  // pop rdi
        0x48, 0xB8, 0x00, 0x00, 0x00, 0x00,    // mov rax, 0x0000000000000000 (original entry)
        0x00, 0x00, 0x00, 0x00,
        0xFF, 0xE0,                            // jmp rax
};

void X86_64TrampolineInstaller::InitTrampolines() {
    kBridgeJumpTrampoline = AS_VOID_PTR(const_cast<unsigned char*>(bridge_jump_trampoline));
    kBridgeJumpTrampolineTargetMethodOffset = 3; // mov rdi, <target_method>
    kBridgeJumpTrampolineExtrasOffset = 14; // mov rdi, <extras>
    kBridgeJumpTrampolineBridgeMethodOffset = 38; // mov rax, <bridge_method>
    kBridgeJumpTrampolineBridgeEntryOffset = 48; // mov rcx, <bridge_entry>
    kBridgeJumpTrampolineOriginCodeEntryOffset = 58; // mov rax, <original_entry> (in jump_to_original)

    kTrampolinesEnd = AS_VOID_PTR(AS_PTR_NUM(bridge_jump_trampoline) + Memory::AlignUp<uintptr_t>(sizeof(bridge_jump_trampoline), 8));

    kCallOriginTrampoline = kTrampolinesEnd; // For calculate size only

    kDirectJumpTrampoline = kBackupTrampoline; // For calculate size only
    kDirectJumpTrampolineSize = kDirectJumpSize;
    kCallOriginTrampolineOriginMethodOffset = 0;
    kCallOriginTrampolineOriginalEntryOffset = 0;

    kBackupTrampoline = kCallOriginTrampoline; // For calculate size only
    kBackupTrampolineOriginMethodOffset = 0;
    kBackupTrampolineOverrideSpaceOffset = 0;
    kBackupTrampolineRemainingCodeEntryOffset = 0;
}

bool X86_64TrampolineInstaller::NativeHookNoBackup(void* target, void* to) {
    bool target_code_writable = Memory::Unprotect(target);
    if (UNLIKELY(!target_code_writable)) {
        LOGE("Failed to make target code %p writable!", target);
        return false;
    }

    {
        WriteDirectJump(target, to);
    }
    return true;
}
