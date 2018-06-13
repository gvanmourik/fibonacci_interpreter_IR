#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>

using namespace llvm;

int main()
{
	static LLVMContext context;
	static IRBuilder<> builder(context);
	Module *module = new Module("myModule", context);
	//static std::unique_ptr<Module> module;

	FunctionType *funcType = FunctionType::get(builder.getVoidTy(), false);
	Function *mainFunc = Function::Create(funcType, Function::ExternalLinkage, "main", module);

	BasicBlock *entry = BasicBlock::Create(context, "entrypoint", mainFunc);
	builder.SetInsertPoint(entry);

	Value *helloWorld = builder.CreateGlobalStringPtr("hello world!\n");

	// puts() Method
	std::vector<Type*> putsArgs;
	putsArgs.push_back(builder.getInt8Ty()->getPointerTo());
	ArrayRef<Type*> argsRef(putsArgs);

	FunctionType *putsType = FunctionType::get(builder.getInt32Ty(), argsRef, false);
	Constant *putsFunc = module->getOrInsertFunction("puts", putsType);

	builder.CreateCall(putsFunc, helloWorld);
	builder.CreateRetVoid();
	module->dump();
}

