//
// Created by canyie on 2020/8/27.
// x86_64 support added for NDK 29
//

#ifndef PINE_X86_64_H
#define PINE_X86_64_H

#include "../trampoline_installer.h"

namespace pine {
    class X86_64TrampolineInstaller final : public TrampolineInstaller {
    public:
        X86_64TrampolineInstaller() : TrampolineInstaller(0, true) {
        }

    protected:
        virtual void InitTrampolines() override ;
        virtual bool CannotBackup(art::ArtMethod* target, size_t size) override {
            return true;
        }
        virtual void* CreateDirectJumpTrampoline(void* to) override {
            FATAL("CreateDirectJumpTrampoline unimplemented");
        }
        virtual void* CreateCallOriginTrampoline(art::ArtMethod* origin, void* original_code_entry) override {
            FATAL("CreateCallOriginTrampoline unimplemented");
        }
        virtual void* Backup(art::ArtMethod* target, size_t size) override {
            FATAL("Backup unimplemented");
        }
        virtual void FillWithNopImpl(void* target, size_t size) override {
            FATAL("FillWithNop unimplemented");
        }

        virtual bool NativeHookNoBackup(void* target, void* to) override ;

    private:
        void WriteDirectJump(void* target, void* jump_to) {
            // mov rax, <addr64>
            // jmp rax
            *static_cast<uint8_t*>(target) = 0x48; // REX.W prefix
            *reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(target) + 1) = 0xB8; // mov rax, imm64
            *reinterpret_cast<void**>(reinterpret_cast<uintptr_t>(target) + 2) = jump_to;
            *reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(target) + 10) = 0xFF; // jmp
            *reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(target) + 11) = 0xE0; // rax
        }

        static constexpr size_t kDirectJumpSize = 12; // mov rax + imm64 + jmp rax
    };
}

#endif //PINE_X86_64_H
