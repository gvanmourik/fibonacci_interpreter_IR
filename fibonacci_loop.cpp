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

	Function *fibFunc = InitFibonacciFnc(context, builder, module, targetFibNum);

	/// Function Pass Manager (optimizer)
	FPM = make_unique<FunctionPassManager>(mainModule.get());

	FPM->add(createInstructionCombiningPass());


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
	GenericValue value = exeEng->runFunction(fibFunc, Args);

	outs() << "\n" << *module;
	outs() << "\n-----------------------------------------\n";
	switch (targetFibNum % 10) {
		case(1): 
			if (targetFibNum == 11)
				outs() << targetFibNum << "th fibonacci number = " << value.IntVal << "\n";
			outs() << targetFibNum << "st fibonacci number = " << value.IntVal << "\n";
			break;
		case(2): 
			if (targetFibNum == 12)
				outs() << targetFibNum << "th fibonacci number = " << value.IntVal << "\n";
			outs() << targetFibNum << "nd fibonacci number = " << value.IntVal << "\n";
			break;
		case(3): 
			if (targetFibNum == 13)
				outs() << targetFibNum << "th fibonacci number = " << value.IntVal << "\n";
			outs() << targetFibNum << "rd fibonacci number = " << value.IntVal << "\n";
			break;
		default: 
			outs() << targetFibNum << "th fibonacci number = " << value.IntVal << "\n";
			break;
	}
	outs() << "-----------------------------------------\n";


	return 0;
}

Function* InitFibonacciFnc(LLVMContext &context, IRBuilder<> &builder, Module* module, int targetFibNum)
{
	Function *fibFunc = 
		cast<Function>( module->getOrInsertFunction("fibFunc", Type::getInt32Ty(context)) );

	Value* zero = ConstantInt::get(builder.getInt32Ty(), 0);
	Value* one = ConstantInt::get(builder.getInt32Ty(), 1);
	Value* two = ConstantInt::get(builder.getInt32Ty(), 2);
	Value* N = ConstantInt::get(builder.getInt32Ty(), targetFibNum);

	/// For loop blocks
	BasicBlock *LoopBB = BasicBlock::Create(context, "loop", fibFunc);
	BasicBlock *ExitLoopBB = BasicBlock::Create(context, "exitLoop", fibFunc);
	/// Nested if/else blocks
	BasicBlock *IfEntryBB = BasicBlock::Create(context, "ifEntry", fibFunc, ExitLoopBB);
	BasicBlock *IfTrueBB = BasicBlock::Create(context, "ifTrue", fibFunc, ExitLoopBB);
	BasicBlock *ElseBB = BasicBlock::Create(context, "else", fibFunc, ExitLoopBB);
	
	/// Variables
	Value *next = zero;	
	Value *first = zero;	
	Value *second = one;
	Value *counter = zero;
	next->setName("next");
	first->setName("first");
	second->setName("second");
	counter->setName("counter");

	/// For loop
	BasicBlock *LoopEntryBB = builder.GetInsertBlock(); // START
	builder.CreateBr(LoopBB);
	builder.SetInsertPoint(LoopBB);

		/// IF
		Value *ifCounterLTTwo = builder.CreateICmpULT(counter, two, "ifStmt");
		builder.CreateCondBr(ifCounterLTTwo, IfTrueBB, ElseBB);
		/// TRUE
		builder.SetInsertPoint(IfTrueBB);
		next = counter;
		/// FALSE
		builder.SetInsertPoint(ElseBB);
		next = BinaryOperator::CreateAdd(first, second, "next_new", ElseBB);
		first = second;
		second = next;

		counter = BinaryOperator::CreateAdd(counter, one, "incrementCounter", LoopBB);
	
	BasicBlock *LoopEndBB = builder.GetInsertBlock(); // END

	// Continue the loop while the counter is less than the target fibonacci number
	Value *ifCounterLTN = builder.CreateICmpULT(counter, N, "forLoopExitCond");
	builder.CreateCondBr(ifCounterLTN, LoopBB, ExitLoopBB);
	
	builder.SetInsertPoint(ExitLoopBB);
	Value *result = next;
	ReturnInst::Create(context, result, ExitLoopBB);

	return fibFunc;
}