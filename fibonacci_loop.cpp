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
	BasicBlock *EntryBB = BasicBlock::Create(context, "entry", FibonacciFnc);
	BasicBlock *LoopEntryBB = BasicBlock::Create(context, "loopEntry", FibonacciFnc);
	BasicBlock *LoopBB = BasicBlock::Create(context, "loop", FibonacciFnc);
		//insert the following BBs as a nested group before the ExitLoopBB
		BasicBlock *IfBB = BasicBlock::Create(context, "if", FibonacciFnc, LoopBB);
		BasicBlock *IfTrueBB = BasicBlock::Create(context, "ifTrue", FibonacciFnc, LoopBB);
		BasicBlock *ElseBB = BasicBlock::Create(context, "else", FibonacciFnc, LoopBB);
		BasicBlock *MergeBB = BasicBlock::Create(context, "merge", FibonacciFnc, LoopBB);
	BasicBlock *ExitLoopBB = BasicBlock::Create(context, "exitLoop", FibonacciFnc);
	
	/// Variables
	Value *next = zero;	
	Value *first = zero;	
	Value *second = one;
	Value *count = zero;
	Value *nextCount = count;
	next->setName("next");
	first->setName("first");
	second->setName("second");
	count->setName("count");
	nextCount->setName("nextCount");

	/// Fall thru branch to EntryBB
	// builder.CreateBr(EntryBB);
	// BranchInst::Create(LoopEntryBB, EntryBB);

	/// EntryBB
	builder.SetInsertPoint(EntryBB);
	// builder.CreateLoad(next, next->getName());
	builder.CreateBr(LoopEntryBB);
	
	/// LoopEntryBB
	builder.SetInsertPoint(LoopEntryBB);
	Value *ifCountLTN = builder.CreateICmpULT(count, N, "enterLoopCond");
	builder.CreateCondBr(ifCountLTN, LoopBB, ExitLoopBB);

	/// LoopBB
	builder.SetInsertPoint(LoopBB);
	builder.CreateBr(IfBB);

		/// IfBB
		builder.SetInsertPoint(IfBB);
		Value *ifCountLTTwo = builder.CreateICmpULT(count, two, "ifStmt");
		count = BinaryOperator::CreateAdd(count, one, "newCount", IfBB); //increment
		builder.CreateCondBr(ifCountLTTwo, IfTrueBB, ElseBB);

		/// IfTrueBB
		builder.SetInsertPoint(IfTrueBB);
		Value *next1 = count;
		next1->setName("next1");
		builder.CreateBr(MergeBB); // terminate IfTrueBB
		IfTrueBB = builder.GetInsertBlock(); // update IfTrue for the phi node

		/// ElseBB
		builder.SetInsertPoint(ElseBB);
		Value *next2 = BinaryOperator::CreateAdd(first, second, "next2", ElseBB);
		first = second;
		second = next2;
		builder.CreateBr(MergeBB); // terminate ElseBB
		ElseBB = builder.GetInsertBlock(); // update ElseBB for the phi node

		/// MergeBB
		builder.SetInsertPoint(MergeBB);
		PHINode *PhiNodeIf = builder.CreatePHI(Type::getInt32Ty(context), 2, "iftmp");
		PhiNodeIf->addIncoming(next1, IfTrueBB);
		PhiNodeIf->addIncoming(next2, ElseBB);
		next = PhiNodeIf; // update "next"
		builder.CreateBr(LoopEntryBB);

	
	builder.SetInsertPoint(ExitLoopBB);
	/// Update and return the final value
	Value *result = next;
	ReturnInst::Create(context, result, ExitLoopBB);


	return FibonacciFnc;
}