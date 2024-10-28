#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

using namespace llvm;

namespace {

// Static variable to track modifications
static bool Modified = false;

// DivPass pass without printing
struct DivisionShifts : public PassInfoMixin<DivisionShifts> {
    PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {
        Modified = false;
        // Iterate over basic blocks in the function
        for (auto &BB : F) {
            // Iterate over instructions in the basic block
            for (auto I = BB.begin(), E = BB.end(); I != E; ) {
                Instruction *Inst = &(*I++);
                // Check if the instruction is a binary operator
                if (auto *UDiv = dyn_cast<BinaryOperator>(Inst)) {
                    // Check if the binary operator is UDiv
                    if (UDiv->getOpcode() == Instruction::UDiv) {
                        // Check if the second operand is a constant power of two
                        if (auto *C = dyn_cast<ConstantInt>(UDiv->getOperand(1))) {
                            if (C->getValue().isPowerOf2()) {
                                // If true, replace div with right shift
                                IRBuilder<> Builder(UDiv);
                                Value *ShiftAmount = Builder.getInt32(C->getValue().exactLogBase2());
                                Value *NewUDiv = Builder.CreateShr(UDiv->getOperand(0), ShiftAmount);
                                UDiv->replaceAllUsesWith(NewUDiv);
                                UDiv->eraseFromParent(); // Remove the div instruction
                                Modified = true;
                            }
                        }
                    }
                }
            }
        }
        // Indicate whether the function was modified or not
        return Modified ? PreservedAnalyses::none() : PreservedAnalyses::all();
    }
};

// New Printer pass
struct DivisionShiftsPrinter : public PassInfoMixin<DivisionShiftsPrinter> {
    PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {
        errs() << "*** Division SHIFTS PASS EXECUTING ***\n";
        if (Modified) {
            errs() << "Some instruction was replaced.\n";
        } else {
            errs() << "Nothing changed.\n";
        }
        return PreservedAnalyses::all();
    }
};
}

// Register the passes as plugins
PassPluginLibraryInfo getDivisionShiftsPluginInfo() {
    return {LLVM_PLUGIN_API_VERSION, "DivisionShifts", LLVM_VERSION_STRING,
            [](PassBuilder &PB) {
                PB.registerPipelineParsingCallback(
                    [](StringRef Name, FunctionPassManager &FPM,
                       ArrayRef<PassBuilder::PipelineElement>) {
                      if (Name == "div-shift") {
                        FPM.addPass(DivisionShifts()); // Run the transformation pass
                        FPM.addPass(DivisionShiftsPrinter()); // Run the printer pass with the modified status
                        return true;
                      }
                      return false;
                    });
                PB.registerPipelineStartEPCallback([](ModulePassManager &MPM,
                                                      OptimizationLevel Level) {
                    FunctionPassManager FPM;
                    FPM.addPass(DivisionShifts()); // Run the transformation pass
                    FPM.addPass(DivisionShiftsPrinter()); // Run the printer pass with the modified status

                    MPM.addPass(createModuleToFunctionPassAdaptor(std::move(FPM)));
                });
            }};
}

// Entry point for the pass plugin
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
    return getDivisionShiftsPluginInfo();
}

