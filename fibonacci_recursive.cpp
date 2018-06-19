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

Function* InitFibonacciFnc(LLVMContext &context, IRBuilder<> &builder, Module* module);
CallInst* createSubCallInst(Argument *X, Value *constInt, std::string argName, BasicBlock *BB,
							Function *fnc, std::string callName);

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

	Function *FibonacciFnc = InitFibonacciFnc( context, builder, module );

	/// Create a JIT
	// auto engine = EngineKind::JIT;
	std::string collectedErrors;
	ExecutionEngine *exeEng = 
		EngineBuilder(std::move(mainModule))
		.setErrorStr(&collectedErrors)
		.setEngineKind(EngineKind::JIT)
		.create();

	if ( !exeEng )
	{
		std::string error = "Unable to construct execution engine: " + collectedErrors;
		perror(error.c_str());
		return -1;
	}

	std::vector<GenericValue> Args(1);
	Args[0].IntVal = APInt(32, targetFibNum);
	GenericValue value = exeEng->runFunction(FibonacciFnc, Args);

	outs() << "\n" << *module;
	outs() << "\n-----------------------------------------\n";
	outs() << targetFibNum << "th fibonacci number = " << value.IntVal << "\n";
	outs() << "-----------------------------------------\n";


	return 0;
}

Function* InitFibonacciFnc(LLVMContext &context, IRBuilder<> &builder, Module* module)
{
	// std::vector<Type*> fncArgs;
	// fncArgs.push_back(builder.getInt8Ty()->getPointerTo());
	// ArrayRef<Type*> argsRef(fncArgs);
	// FunctionType *fncType = FunctionType::get(builder.getInt32Ty(), argsRef, false);
	// Function *FibonacciFnc = Function::Create(fncType, Function::ExternalLinkage, "FibonacciFnc", module);
	Function *FibonacciFnc = cast<Function>(module->getOrInsertFunction("FibonacciFnc", 
																	Type::getInt32Ty(context),
																	Type::getInt32Ty(context)
																	));

	Value* one = ConstantInt::get(builder.getInt32Ty(), 1);
	Value* two = ConstantInt::get(builder.getInt32Ty(), 2);

	BasicBlock *EntryBB = BasicBlock::Create(context, "entry", FibonacciFnc);
	BasicBlock *ContinueBB = BasicBlock::Create(context, "continue", FibonacciFnc);
	BasicBlock *ExitBB = BasicBlock::Create(context, "exit", FibonacciFnc);
	
	Argument *X = &*FibonacciFnc->arg_begin();
	X->setName("X_Arg");
	// Value* X_value = dyn_cast<ConstantInt>(X);

	/// BASE case
	Value *ifCond = new ICmpInst(*EntryBB, ICmpInst::ICMP_SLE, X, two, "if_cond");
	BranchInst::Create(ExitBB, ContinueBB, ifCond, EntryBB);

	// Return instruction
	ReturnInst::Create(context, one, ExitBB);
	///

	/// RECURSIVE case
	CallInst *callFib1 = createSubCallInst(X, one, "", ContinueBB,
										FibonacciFnc, "fib1");
	CallInst *callFib2 = createSubCallInst(X, two, "", ContinueBB,
										FibonacciFnc, "fib2");
	Value *sum = BinaryOperator::CreateAdd(callFib1, callFib2, "sum", ContinueBB);
	
	// Return instruction
	ReturnInst::Create(context, sum, ContinueBB);
	///

	return FibonacciFnc;
}

CallInst* createSubCallInst(Argument *X, Value *constInt, std::string argName, BasicBlock *BB,
							Function *fnc, std::string callName)
{
	Value *difference = BinaryOperator::CreateSub(X, constInt, argName, BB);
	CallInst *call = CallInst::Create(fnc, difference, callName, BB);
	call->setTailCall();

	return call;
}

