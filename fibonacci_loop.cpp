#include <llvm/ADT/APInt.h>
#include <llvm/IR/Verifier.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/IR/Argument.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <algorithm>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>
#include <iostream>


using namespace llvm;


Function* InitFibonacciFnc(LLVMContext &context, IRBuilder<> &builder, Module* module, int targetFibNum);


int main(int argc, char* argv[])
{
	/// Collect N, for Nth fibonacci number
	if ( argv[1] == nullptr )
	{
		perror("No argument entered for the Nth fibonacci number");
		return -1;
	}
	if ( argc > 2 )
	{
		perror("Only the first argument was used");
	}

	/// Convert and check
	int targetFibNum = atol(argv[1]);
	if ( targetFibNum > 29 )
	{
		perror("Argument passed was too large");
		return -1;
	}
	
	/// LLVM IR Variables
	static LLVMContext context;
	static IRBuilder<> builder(context);
	std::unique_ptr<Module> mainModule( new Module("fibonacciModule", context) );
	Module *module = mainModule.get();
	InitializeNativeTarget();
	InitializeNativeTargetAsmPrinter();

	Function *FibonacciFnc = InitFibonacciFnc(context, builder, module, targetFibNum);

	/// Create a JIT
	std::string collectedErrors;
	ExecutionEngine *exeEng = 
		EngineBuilder(std::move(mainModule))
		.setErrorStr(&collectedErrors)
		.setEngineKind(EngineKind::JIT)
		.create();

	/// Execution Engine
	if ( !exeEng )
	{
		std::string error = "Unable to construct execution engine: " + collectedErrors;
		perror(error.c_str());
		return -1;
	}

	std::vector<GenericValue> Args(0); // Empty vector as no args are passed
	GenericValue value = exeEng->runFunction(FibonacciFnc, Args);

	outs() << "\n" << *module;
	outs() << "\n-----------------------------------------\n";
	outs() << targetFibNum << "th fibonacci number = " << value.IntVal << "\n";
	outs() << "-----------------------------------------\n";


	return 0;
}

Function* InitFibonacciFnc(LLVMContext &context, IRBuilder<> &builder, Module* module, int targetFibNum)
{
	Function *FibonacciFnc = 
		cast<Function>( module->getOrInsertFunction("FibonacciFnc", Type::getInt32Ty(context)) );

	Value* zero = ConstantInt::get(builder.getInt32Ty(), 0);
	Value* one = ConstantInt::get(builder.getInt32Ty(), 1);
	Value* two = ConstantInt::get(builder.getInt32Ty(), 2);
	Value* N = ConstantInt::get(builder.getInt32Ty(), targetFibNum);

	/// For loop blocks
	BasicBlock *LoopBB = BasicBlock::Create(context, "loop", FibonacciFnc);
	BasicBlock *ExitLoopBB = BasicBlock::Create(context, "exitLoop", FibonacciFnc);
	/// Nested if/else blocks
	// BasicBlock *IfEntryBB = BasicBlock::Create(context, "ifEntry", FibonacciFnc, ExitLoopBB);
	BasicBlock *IfTrueBB = BasicBlock::Create(context, "ifTrue");
	BasicBlock *ElseBB = BasicBlock::Create(context, "else");
	BasicBlock *MergeBB = BasicBlock::Create(context, "merge");
	
	/// Variables
	Value *next = zero;	
	Value *first = zero;	
	Value *second = one;
	Value *count = zero;
	next->setName("next");
	first->setName("first");
	second->setName("second");
	count->setName("count");

	/// For loop *********************************START*********************************
	BasicBlock *LoopEntryBB = builder.GetInsertBlock();
	builder.CreateBr(LoopBB);
	builder.SetInsertPoint(LoopBB);

	PHINode *PhiNodeFor = builder.CreatePHI(Type::getInt32Ty(context), 2, count->getName());
	PhiNodeFor->addIncoming(count, LoopEntryBB);

	// Value *oldCount = count;
	// count = PhiNodeFor;

		/// If entry
		Value *ifcountLTTwo = builder.CreateICmpULT(count, two, "ifStmt");
		builder.CreateCondBr(ifcountLTTwo, IfTrueBB, ElseBB);
		
		/// If
		FibonacciFnc->getBasicBlockList().push_back(IfTrueBB);
		builder.SetInsertPoint(IfTrueBB);
		Value *nextIf = count;
		builder.CreateBr(MergeBB); // terminate IfTrueBB
		IfTrueBB = builder.GetInsertBlock(); // update IfTrue for the phi node
		
		/// Else
		FibonacciFnc->getBasicBlockList().push_back(ElseBB);
		builder.SetInsertPoint(ElseBB);
		Value *nextElse = BinaryOperator::CreateAdd(first, second, "next_new", ElseBB);
		first = second;
		second = nextElse;
		builder.CreateBr(MergeBB); // terminate ElseBB
		ElseBB = builder.GetInsertBlock(); // update ElseBB for the phi node

		/// Merge
		FibonacciFnc->getBasicBlockList().push_back(MergeBB);
		builder.SetInsertPoint(MergeBB);
		PHINode *PhiNodeIf = builder.CreatePHI(Type::getInt32Ty(context), 2, "iftmp");
		PhiNodeIf->addIncoming(nextIf, IfTrueBB);
		PhiNodeIf->addIncoming(nextElse, ElseBB);
		next = PhiNodeIf; // update "next"


	// Value *StepValue = ConstantFP::get(context, APFloat(1.0));
	Value *nextCount = BinaryOperator::CreateAdd(count, one, "nextCount", LoopBB);
	
	BasicBlock *LoopEndBB = builder.GetInsertBlock();

	// Continue the loop while the count is less than the target fibonacci number
	Value *ifCountLTN = builder.CreateICmpULT(count, N, "forLoopExitCond");
	builder.CreateCondBr(ifCountLTN, LoopBB, ExitLoopBB);
	
	builder.SetInsertPoint(ExitLoopBB);
	PhiNodeFor->addIncoming(nextCount, ExitLoopBB);

	// if (oldCount)
	// 	count;
	// else
	// 	NamedValues.erase(count.getName());
	/// For loop *******************************END*************************************

	/// Update and return the final value of "next"
	Value *result = next;
	ReturnInst::Create(context, result, ExitLoopBB);

	return FibonacciFnc;
}