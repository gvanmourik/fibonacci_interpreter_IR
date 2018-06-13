#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Constants.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <iostream>

using namespace llvm;


Function* InitFibonacciFnc(LLVMContext &context, IRBuilder<> &builder, Module* module);
CallInst* createCallInst(Argument *Arg, Value *v, std::string argName, BasicBlock *BB,
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
	std::unique_ptr<Module> module( new Module("fibonacciModule", context) );
	// Module* module = new Module("fibonacciModule", context);
	InitializeNativeTarget(); // for JIT compilation

	Function *fibFunc = InitFibonacciFnc( context, builder, module.get() );

	/// Create a JIT
	// auto engine = EngineKind::JIT;
	std::string collectedErrors;
	ExecutionEngine *exeEng = 
		EngineBuilder(std::move(module))
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
	GenericValue value = exeEng->runFunction(fibFunc, Args);

	outs() << targetFibNum << "th fibonacci number = " << value.IntVal << "\n";


	return 0;
}

Function* InitFibonacciFnc(LLVMContext &context, IRBuilder<> &builder, Module* module)
{
	std::vector<Type*> fncArgs;
	fncArgs.push_back(builder.getInt8Ty()->getPointerTo());
	ArrayRef<Type*> argsRef(fncArgs);
	FunctionType *fncType = FunctionType::get(builder.getInt32Ty(), argsRef, false);
	Function *fibFunc = Function::Create(fncType, Function::ExternalLinkage, "fibFunc", module);

	Value* one = ConstantInt::get(builder.getInt32Ty(), 1);
	Value* two = ConstantInt::get(builder.getInt32Ty(), 2);

	BasicBlock *EntryBB = BasicBlock::Create(context, "entry", fibFunc);
	BasicBlock *ContinueBB = BasicBlock::Create(context, "continue", fibFunc);
	BasicBlock *ExitBB = BasicBlock::Create(context, "exit", fibFunc);
	
	Argument *IntArg = fibFunc->arg_begin();
	IntArg->setName("FibIntArg");

	/// BASE case
	// Return instruction
	ReturnInst::Create(context, fib1, ExitBB);
	///

	/// RECURSIVE case
	CallInst *callFib1 = createCallInst(IntArg, one, "arg", ContinueBB,
										fibFunc, "fib1");
	CallInst *callFib2 = createCallInst(IntArg, two, "arg", ContinueBB,
										fibFunc, "fib2");
	Value *fibSum = BinaryOperator::CreateAdd(callFib1, callFib2, "fibSum", ContinueBB);
	// Return instruction
	ReturnInst::Create(context, fibSum, ContinueBB);
	///

	return fibFunc;
}

CallInst* createCallInst(Argument *Arg, Value *v, std::string argName, BasicBlock *BB,
							Function *fnc, std::string callName)
{
	Value *sub = BinaryOperator::CreateSub(Arg, v, argName, BB);
	CallInst *call = CallInst::Create(fnc, sub, callName, BB);
	call->setTailCall();

	return call;
}

