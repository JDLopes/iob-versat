#include "versatPrivate.hpp"

#include <new>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cstdarg>
#include <unordered_map>

#include "printf.h"

#include "thread.hpp"
#include "type.hpp"
#include "debug.hpp"
#include "parser.hpp"
#include "configurations.hpp"
#include "debugGUI.hpp"
#include "graph.hpp"
#include "acceleratorSimulation.hpp"

#include "templateEngine.hpp"

static int zeros[99] = {};
static Array<int> zerosArray = {zeros,99};

#define IMPLEMENT_VERILOG_UNITS
#include "basicWrapper.inc"
#include "verilogWrapper.inc"
#include "templateData.inc"

static int ones[64] = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};
static int* DefaultInitFunction(ComplexFUInstance* inst){
   inst->done = true;
   return nullptr;
}
static int* DefaultUpdateFunction(ComplexFUInstance* inst,Array<int> inputs){
   static int out[99];

   for(int i = 0; i < inputs.size; i++){
      out[i] = inputs[i];
   }

   return out;
}

static FUDeclaration* RegisterCircuitInput(Versat* versat){
   FUDeclaration decl = {};

   decl.name = STRING("CircuitInput");
   decl.inputDelays = Array<int>{zeros,0};
   decl.outputLatencies = Array<int>{zeros,1};
   decl.initializeFunction = DefaultInitFunction;
   decl.delayType = DelayType::DELAY_TYPE_SOURCE_DELAY;
   decl.type = FUDeclaration::SPECIAL;

   return RegisterFU(versat,decl);
}
static FUDeclaration* RegisterCircuitOutput(Versat* versat){
   FUDeclaration decl = {};

   decl.name = STRING("CircuitOutput");
   decl.inputDelays = Array<int>{zeros,50};
   decl.outputLatencies = Array<int>{zeros,0};
   decl.initializeFunction = DefaultInitFunction;
   decl.delayType = DelayType::DELAY_TYPE_SINK_DELAY;
   decl.type = FUDeclaration::SPECIAL;

   return RegisterFU(versat,decl);
}

static FUDeclaration* RegisterData(Versat* versat){
   FUDeclaration decl = {};

   decl.name = STRING("Data");
   decl.inputDelays = Array<int>{zeros,50};
   decl.outputLatencies = Array<int>{ones,50};
   decl.initializeFunction = DefaultInitFunction;
   decl.updateFunction = DefaultUpdateFunction;
   decl.delayType = DelayType::DELAY_TYPE_SINK_DELAY;

   return RegisterFU(versat,decl);
}
static int* UpdatePipelineRegister(ComplexFUInstance* inst,Array<int> inputs){
   static int out;

   out = inputs[0];
   inst->done = true;

   return &out;
}
static FUDeclaration* RegisterPipelineRegister(Versat* versat){
   FUDeclaration decl = {};
   static int ones[] = {1};

   decl.name = STRING("PipelineRegister");
   decl.inputDelays = Array<int>{zeros,1};
   decl.outputLatencies = Array<int>{ones,1};
   decl.initializeFunction = DefaultInitFunction;
   decl.updateFunction = UpdatePipelineRegister;
   decl.isOperation = true;
   decl.operation = "{0}_{1} <= {2}";

   return RegisterFU(versat,decl);
}

static int* LiteralStartFunction(ComplexFUInstance* inst){
   static int out;
   out = inst->literal;
   inst->done = true;
   return &out;
}
static int* LiteraltUpdateFunction(ComplexFUInstance* inst,Array<int> inputs){
   static int out;
   out = inst->literal;
   return &out;
}

static FUDeclaration* RegisterLiteral(Versat* versat){
   FUDeclaration decl = {};

   decl.name = STRING("Literal");
   decl.outputLatencies = Array<int>{zeros,1};
   decl.startFunction = LiteralStartFunction;
   decl.updateFunction = LiteraltUpdateFunction;

   return RegisterFU(versat,decl);
}

static int* UnaryNot(ComplexFUInstance* inst,Array<int> inputs){
   static unsigned int out;
   out = ~inputs[0];
   inst->done = true;
   return (int*) &out;
}
static int* BinaryXOR(ComplexFUInstance* inst,Array<int> inputs){
   static unsigned int out;
   out = inputs[0] ^ inputs[1];
   inst->done = true;
   return (int*) &out;
}
static int* BinaryADD(ComplexFUInstance* inst,Array<int> inputs){
   static unsigned int out;
   out = inputs[0] + inputs[1];
   inst->done = true;
   return (int*) &out;
}
static int* BinarySUB(ComplexFUInstance* inst,Array<int> inputs){
   static unsigned int out;
   out = inputs[0] - inputs[1];
   inst->done = true;
   return (int*) &out;
}
static int* BinaryAND(ComplexFUInstance* inst,Array<int> inputs){
   static unsigned int out;
   out = inputs[0] & inputs[1];
   inst->done = true;
   return (int*) &out;
}
static int* BinaryOR(ComplexFUInstance* inst,Array<int> inputs){
   static unsigned int out;
   out = inputs[0] | inputs[1];
   inst->done = true;
   return (int*) &out;
}
static int* BinaryRHR(ComplexFUInstance* inst,Array<int> inputs){
   static unsigned int out;
   unsigned int value = inputs[0];
   unsigned int shift = inputs[1];
   out = (value >> shift) | (value << (32 - shift));
   inst->done = true;
   return (int*) &out;
}
static int* BinaryRHL(ComplexFUInstance* inst,Array<int> inputs){
   static unsigned int out;
   unsigned int value = inputs[0];
   unsigned int shift = inputs[1];
   out = (value << shift) | (value >> (32 - shift));
   inst->done = true;
   return (int*) &out;
}
static int* BinarySHR(ComplexFUInstance* inst,Array<int> inputs){
   static unsigned int out;
   unsigned int value = inputs[0];
   unsigned int shift = inputs[1];
   out = (value >> shift);
   inst->done = true;
   return (int*) &out;
}
static int* BinarySHL(ComplexFUInstance* inst,Array<int> inputs){
   static unsigned int out;
   unsigned int value = inputs[0];
   unsigned int shift = inputs[1];
   out = (value << shift);
   inst->done = true;
   return (int*) &out;
}

static void RegisterOperators(Versat* versat){
   struct Operation{
      const char* name;
      FUUpdateFunction function;
      const char* operation;
   };

   Operation unary[] =  {{"NOT",UnaryNot,"{0}_{1} = ~{2}"}};
   Operation binary[] = {{"XOR",BinaryXOR,"{0}_{1} = {2} ^ {3}"},
                         {"ADD",BinaryADD,"{0}_{1} = {2} + {3}"},
                         {"SUB",BinarySUB,"{0}_{1} = {2} - {3}"},
                         {"AND",BinaryAND,"{0}_{1} = {2} & {3}"},
                         {"OR" ,BinaryOR ,"{0}_{1} = {2} | {3}"},
                         {"RHR",BinaryRHR,"{0}_{1} = ({2} >> {3}) | ({2} << (32 - {3}))"},
                         {"SHR",BinarySHR,"{0}_{1} = {2} >> {3}"},
                         {"RHL",BinaryRHL,"{0}_{1} = ({2} << {3}) | ({2} >> (32 - {3}))"},
                         {"SHL",BinarySHL,"{0}_{1} = {2} << {3}"}};

   FUDeclaration decl = {};
   decl.inputDelays = Array<int>{zeros,1};
   decl.outputLatencies = Array<int>{zeros,1};
   decl.isOperation = true;

   for(unsigned int i = 0; i < ARRAY_SIZE(unary); i++){
      decl.name = STRING(unary[i].name);
      decl.updateFunction = unary[i].function;
      decl.operation = unary[i].operation;
      RegisterFU(versat,decl);
   }

   decl.inputDelays = Array<int>{zeros,2};
   for(unsigned int i = 0; i < ARRAY_SIZE(binary); i++){
      decl.name = STRING(binary[i].name);
      decl.updateFunction = binary[i].function;
      decl.operation = binary[i].operation;
      RegisterFU(versat,decl);
   }
}

static Pool<FUDeclaration> basicDeclarations;
namespace BasicDeclaration{
	FUDeclaration* buffer;
   FUDeclaration* fixedBuffer;
   FUDeclaration* input;
   FUDeclaration* output;
   FUDeclaration* multiplexer;
   FUDeclaration* combMultiplexer;
   FUDeclaration* pipelineRegister;
   FUDeclaration* data;
}

namespace BasicTemplates{
   CompiledTemplate* acceleratorTemplate;
   CompiledTemplate* topAcceleratorTemplate;
   CompiledTemplate* dataTemplate;
   CompiledTemplate* externalPortmapTemplate;
   CompiledTemplate* externalPortTemplate;
   CompiledTemplate* externalInstTemplate;
}

Versat* InitVersat(int base,int numberConfigurations){
   static Versat versatInst = {};
   static bool doneOnce = false;

   Assert(!doneOnce); // For now, only allow one Versat instance
   doneOnce = true;

   InitDebug();
   RegisterTypes();

   //InitThreadPool(16);
   Versat* versat = &versatInst;

   versat->debug.outputAccelerator = true;
   versat->debug.outputVersat = true;

   versat->debug.dotFormat = GRAPH_DOT_FORMAT_NAME;

   //versat->debug.dotFormat = GRAPH_DOT_FORMAT_NAME |
                             //GRAPH_DOT_FORMAT_ID |
                             //GRAPH_DOT_FORMAT_DELAY |
                             //GRAPH_DOT_FORMAT_TYPE |
                             //GRAPH_DOT_FORMAT_EXPLICIT |
                             //GRAPH_DOT_FORMAT_LATENCY;

   versat->numberConfigurations = numberConfigurations;
   versat->base = base;

   #ifdef x86
   versat->temp = InitArena(Megabyte(256));
   #else
   versat->temp = InitArena(Gigabyte(8));
   #endif // x86
   versat->permanent = InitArena(Megabyte(256));

   // Technically, if more than 1 versat in the future, could move this outside
   BasicTemplates::acceleratorTemplate = CompileTemplate(versat_accelerator_template,&versat->permanent);
   BasicTemplates::topAcceleratorTemplate = CompileTemplate(versat_top_instance_template,&versat->permanent);
   BasicTemplates::dataTemplate = CompileTemplate(embedData,&versat->permanent);
   BasicTemplates::externalPortmapTemplate = CompileTemplate(external_memory_portmap,&versat->permanent);
   BasicTemplates::externalPortTemplate = CompileTemplate(external_memory_port,&versat->permanent);
   BasicTemplates::externalInstTemplate = CompileTemplate(external_memory_inst,&versat->permanent);

   FUDeclaration nullDeclaration = {};
   nullDeclaration.inputDelays = Array<int>{zeros,1};
   nullDeclaration.outputLatencies = Array<int>{zeros,1};

   // Kinda of a hack, for now. We register on versat and then move the basic declarations out
   RegisterFU(versat,nullDeclaration);
   RegisterAllVerilogUnitsBasic(versat);
   RegisterOperators(versat);
   RegisterPipelineRegister(versat);
   RegisterCircuitInput(versat);
   RegisterCircuitOutput(versat);
   RegisterData(versat);
   RegisterLiteral(versat);

   for(FUDeclaration* decl : versat->declarations){
      FUDeclaration* newSpace = basicDeclarations.Alloc();
      *newSpace = *decl;
   }
   versat->declarations.Clear(false);

   // This ones are specific for the instance
   RegisterAllVerilogUnitsVerilog(versat);

   BasicDeclaration::buffer = GetTypeByName(versat,STRING("Buffer"));
   BasicDeclaration::fixedBuffer = GetTypeByName(versat,STRING("FixedBuffer"));
   BasicDeclaration::pipelineRegister = GetTypeByName(versat,STRING("PipelineRegister"));
   BasicDeclaration::multiplexer = GetTypeByName(versat,STRING("Mux2"));
   BasicDeclaration::combMultiplexer = GetTypeByName(versat,STRING("CombMux2"));
   BasicDeclaration::input = GetTypeByName(versat,STRING("CircuitInput"));
   BasicDeclaration::output = GetTypeByName(versat,STRING("CircuitOutput"));
   BasicDeclaration::data = GetTypeByName(versat,STRING("Data"));

   Log(LogModule::TOP_SYS,LogLevel::INFO,"Init versat");

   return versat;
}

void Free(Versat* versat){
   #if 0
   Arena* arena = &versat->temp;
   for(Accelerator* accel : versat->accelerators){
      #if 0
      if(accel->type == Accelerator::CIRCUIT){
         continue;
      }
      #endif

      // Cannot free this way because most of accelerators are bound to a FUDeclaration but are incomplete.
      // Therefore
      AcceleratorIterator iter = {};
      for(InstanceNode* node = iter.Start(accel,arena,true); node; node = iter.Next()){
         ComplexFUInstance* inst = node->inst;
         if(inst->declaration->destroyFunction){
            inst->declaration->destroyFunction(inst);
         }
      }
   }
   #endif

   for(Accelerator* accel : versat->accelerators){
      Free(&accel->configAlloc);
      Free(&accel->stateAlloc);
      Free(&accel->delayAlloc);
      Free(&accel->staticAlloc);
      Free(&accel->extraDataAlloc);
      Free(&accel->externalMemoryAlloc);
      Free(&accel->debugDataAlloc);
      Free(&accel->outputAlloc);
      Free(&accel->storedOutputAlloc);

      accel->staticUnits.clear();
   }

   versat->accelerators.Clear(true);
   versat->declarations.Clear(true);

   Free(&versat->temp);
   Free(&versat->permanent);

   FreeTypes();

   CheckMemoryStats();
}

void ParseCommandLineOptions(Versat* versat,int argc,const char** argv){
   #if 0
   for(int i = 0; i < argc; i++){
      printf("Arg %d: %s\n",i,argv[i]);
   }
   #endif
}

uint SetDebug(Versat* versat,VersatDebugFlags flags,uint flag){
   uint last;
   switch(flags){
   case VersatDebugFlags::OUTPUT_GRAPH_DOT:{
      last = versat->debug.outputGraphs;
      versat->debug.outputGraphs = flag;
   }break;
   case VersatDebugFlags::GRAPH_DOT_FORMAT:{
      last = versat->debug.dotFormat;
      versat->debug.dotFormat = flag;
   }break;
   case VersatDebugFlags::OUTPUT_ACCELERATORS_CODE:{
      last = versat->debug.outputAccelerator;
      versat->debug.outputAccelerator = flag;
   }break;
   case VersatDebugFlags::OUTPUT_VERSAT_CODE:{
      last = versat->debug.outputVersat;
      versat->debug.outputVersat = flag;
   }break;
   case VersatDebugFlags::OUTPUT_VCD:{
      last = versat->debug.outputVCD;
      versat->debug.outputVCD = flag;
   }break;
   case VersatDebugFlags::USE_FIXED_BUFFERS:{
      last = versat->debug.useFixedBuffers;
      versat->debug.useFixedBuffers = flag;
   }break;
   default:{
      NOT_POSSIBLE;
   }break;
   }

   return last;
}

void SetDefaultConfiguration(FUInstance* instance){
   ComplexFUInstance* inst = (ComplexFUInstance*) instance;

   inst->savedConfiguration = true;
}

void ShareInstanceConfig(FUInstance* instance, int shareBlockIndex){
   ComplexFUInstance* inst = (ComplexFUInstance*) instance;

   inst->sharedIndex = shareBlockIndex;
   inst->sharedEnable = true;
}

void SetStatic(Accelerator* accel,FUInstance* instance){
   ComplexFUInstance* inst = (ComplexFUInstance*) instance;

   int size = inst->declaration->configs.size;
   iptr* oldConfig = inst->config;

   StaticId id = {};
   id.name = inst->name;

   auto iter = accel->staticUnits.find(id);
   if(iter == accel->staticUnits.end()){
      bool repopulate = false;
      if(accel->staticAlloc.size + size > accel->staticAlloc.reserved){
         int oldSize = accel->staticAlloc.size;
         ZeroOutRealloc(&accel->staticAlloc,accel->staticAlloc.size + size);
         accel->staticAlloc.size = oldSize;

         repopulate = true;
      }

      iptr* ptr = Push(&accel->staticAlloc,size);

      StaticData data = {};
      data.offset = ptr - accel->staticAlloc.ptr;

      accel->staticUnits.insert({id,data});

      // Only copy data if new static. No reason, do not know if always copying it would be better or not
      Memcpy(ptr,inst->config,size);
      inst->config = ptr;

      if(repopulate){
         Arena* arena = &accel->versat->temp;
         BLOCK_REGION(arena);
         AcceleratorIterator iter = {};
         iter.Start(accel,arena,true);
      }
   }

   inst->isStatic = true;

   RemoveChunkAndCompress(&accel->configAlloc,oldConfig,size);
}

static int CompositeMemoryAccess(ComplexFUInstance* instance,int address,int value,int write){
   int offset = 0;

   // TODO: Probably should use Accelerator Iterator ?
   FOREACH_LIST(ptr,instance->declaration->fixedDelayCircuit->allocated){
      ComplexFUInstance* inst = ptr->inst;

      if(!inst->declaration->isMemoryMapped){
         continue;
      }

      int mappedWords = 1 << inst->declaration->memoryMapBits;
      if(mappedWords){
         if(address >= offset && address <= offset + mappedWords){
            if(write){
               VersatUnitWrite(inst,address - offset,value);
               return 0;
            } else {
               int result = VersatUnitRead(inst,address - offset);
               return result;
            }
         } else {
            offset += mappedWords;
         }
      }
   }

   NOT_POSSIBLE; // TODO: Figure out what to do in this situation
   return 0;
}

static int AccessMemory(FUInstance* inst,int address, int value, int write){
   ComplexFUInstance* instance = (ComplexFUInstance*) inst->declarationInstance;
   instance->memMapped = inst->memMapped;
   instance->config = inst->config;
   instance->state = inst->state;
   instance->delay = inst->delay;
   instance->externalMemory = inst->externalMemory;
   instance->outputs = inst->outputs;
   instance->storedOutputs = inst->storedOutputs;
   instance->extraData = inst->extraData;

   int res = instance->declaration->memAccessFunction(instance,address,value,write);

   return res;
}

void VersatUnitWrite(FUInstance* instance,int address, int value){
   AccessMemory(instance,address,value,1);
}

int VersatUnitRead(FUInstance* instance,int address){
   int res = AccessMemory(instance,address,0,0);
   return res;
}

FUDeclaration* GetTypeByName(Versat* versat,String name){
   for(FUDeclaration* decl : versat->declarations){
      if(CompareString(decl->name,name)){
         return decl;
      }
   }

   for(FUDeclaration* decl : basicDeclarations){
      if(CompareString(decl->name,name)){
         return decl;
      }
   }

   Log(LogModule::TOP_SYS,LogLevel::FATAL,"[GetTypeByName] Didn't find the following type: %.*s",UNPACK_SS(name));

   return nullptr;
}

static FUInstance* GetInstanceByHierarchicalName(Accelerator* accel,HierarchicalName* hier){
   Assert(hier != nullptr);

   HierarchicalName* savedHier = hier;
   FUInstance* res = nullptr;
   FOREACH_LIST(ptr,accel->allocated){
      ComplexFUInstance* inst = ptr->inst;
      Tokenizer tok(inst->name,"./",{});

      while(true){
         // Unpack individual name
         hier = savedHier;
         while(true){
            Token name = tok.NextToken();

            // Unpack hierarchical name
            Tokenizer hierTok(hier->name,":",{});
            Token hierName = hierTok.NextToken();

            if(!CompareString(name,hierName)){
               break;
            }

            Token possibleTypeQualifier = hierTok.PeekToken();

            if(CompareString(possibleTypeQualifier,":")){
               hierTok.AdvancePeek(possibleTypeQualifier);

               Token type = hierTok.NextToken();

               if(!CompareString(type,inst->declaration->name)){
                  break;
               }
            }

            Token possibleDot = tok.PeekToken();

            // If hierarchical name, need to advance through hierarchy
            if(CompareString(possibleDot,".") && hier->next){
               tok.AdvancePeek(possibleDot);
               Assert(hier); // Cannot be nullptr

               hier = hier->next;
               continue;
            } else if(inst->declaration->fixedDelayCircuit && hier->next){
               FUInstance* res = GetInstanceByHierarchicalName(inst->declaration->fixedDelayCircuit,hier->next);
               if(res){
                  return res;
               }
            } else if(!hier->next){ // Correct name and type (if specified) and no further hierarchical name to follow
               return inst;
            }
         }

         // Check if multiple names
         Token possibleDuplicateName = tok.PeekFindIncluding("/");
         if(possibleDuplicateName.size > 0){
            tok.AdvancePeek(possibleDuplicateName);
         } else {
            break;
         }
      }
   }

   return res;
}

static FUInstance* GetInstanceByHierarchicalName2(AcceleratorIterator iter,HierarchicalName* hier,Arena* arena){
   HierarchicalName* savedHier = hier;
   FUInstance* res = nullptr;
   for(InstanceNode* node = iter.Current(); node; node = iter.Skip()){
      ComplexFUInstance* inst = node->inst;
      Tokenizer tok(inst->name,"./",{});

      while(true){
         // Unpack individual name
         hier = savedHier;
         while(true){
            Token name = tok.NextToken();

            // Unpack hierarchical name
            Tokenizer hierTok(hier->name,":",{});
            Token hierName = hierTok.NextToken();

            if(!CompareString(name,hierName)){
               break;
            }

            Token possibleTypeQualifier = hierTok.PeekToken();

            if(CompareString(possibleTypeQualifier,":")){
               hierTok.AdvancePeek(possibleTypeQualifier);

               Token type = hierTok.NextToken();

               if(!CompareString(type,inst->declaration->name)){
                  break;
               }
            }

            Token possibleDot = tok.PeekToken();

            // If hierarchical name, need to advance through hierarchy
            if(CompareString(possibleDot,".") && hier->next){
               tok.AdvancePeek(possibleDot);
               Assert(hier); // Cannot be nullptr

               hier = hier->next;
               continue;
            } else if(inst->declaration->fixedDelayCircuit && hier->next){
               AcceleratorIterator it = iter.LevelBelowIterator(arena);
               FUInstance* res = GetInstanceByHierarchicalName2(it,hier->next,arena);
               if(res){
                  return res;
               }
            } else if(!hier->next){ // Correct name and type (if specified) and no further hierarchical name to follow
               Accelerator* accel = iter.topLevel;

               #if 0
               if(iter.level == 0){
                  return node->inst;
               } else {
               #endif
                  FUInstance* instBuffer = accel->subInstances.Alloc();
                  *instBuffer = *((FUInstance*)inst);
                  instBuffer->declarationInstance = inst;

                  return instBuffer;
               //}
            }
         }

         // Check if multiple names
         Token possibleDuplicateName = tok.PeekFindIncluding("/");
         if(possibleDuplicateName.size > 0){
            tok.AdvancePeek(possibleDuplicateName);
         } else {
            break;
         }
      }
   }

   return res;
}

static FUInstance* vGetInstanceByName_(AcceleratorIterator iter,Arena* arena,int argc,va_list args){
   HierarchicalName fullName = {};
   HierarchicalName* namePtr = &fullName;
   HierarchicalName* lastPtr = nullptr;

   for (int i = 0; i < argc; i++){
      char* str = va_arg(args, char*);
      int arguments = parse_printf_format(str,0,nullptr);

      if(namePtr == nullptr){
         HierarchicalName* newBlock = PushStruct<HierarchicalName>(arena);
         *newBlock = {};

         lastPtr->next = newBlock;
         namePtr = newBlock;
      }

      String name = {};
      if(arguments){
         name = vPushString(arena,str,args);
         i += arguments;
         #ifdef x86 // For some reason this is needed for 32 bits but not for 64 bits. Hack for now, but need to do a full debug and see what is happening
         for(int ii = 0; ii < arguments; ii++){
            /* int unused = */ va_arg(args, int); // Need to consume something
         }
         #endif
      } else {
         name = PushString(arena,"%s",str);
      }

      Tokenizer tok(name,".",{});
      while(!tok.Done()){
         if(namePtr == nullptr){
            HierarchicalName* newBlock = PushStruct<HierarchicalName>(arena);
            *newBlock = {};

            lastPtr->next = newBlock;
            namePtr = newBlock;
         }

         Token name = tok.NextToken();

         namePtr->name = name;
         lastPtr = namePtr;
         namePtr = namePtr->next;

         Token peek = tok.PeekToken();
         if(CompareString(peek,".")){
            tok.AdvancePeek(peek);
            continue;
         }

         break;
      }
   }

   FUInstance* res = GetInstanceByHierarchicalName2(iter,&fullName,arena);

   if(!res){
      printf("Couldn't find: ");
      bool first = true;
      FOREACH_LIST(ptr,&fullName){
         if(!first){
            printf(":");
         }
         printf("%.*s",UNPACK_SS(ptr->name));
         first = false;
      }
      printf("\n");
      Assert(res);
   }

   return res;
}

FUInstance* GetInstanceByName_(Accelerator* circuit,int argc, ...){
   Arena* temp = &circuit->versat->temp;
   BLOCK_REGION(temp);

   va_list args;
   va_start(args,argc);

   AcceleratorIterator iter = {};
   iter.Start(circuit,temp,true);

   FUInstance* res = vGetInstanceByName_(iter,temp,argc,args);

   va_end(args);

   return res;
}

FUInstance* GetSubInstanceByName_(Accelerator* topLevel,FUInstance* instance,int argc, ...){
   Arena* temp = &topLevel->versat->temp;
   BLOCK_REGION(temp);

   va_list args;
   va_start(args,argc);

   ComplexFUInstance* inst = (ComplexFUInstance*) instance;
   Assert(inst->declaration->fixedDelayCircuit);

   AcceleratorIterator iter = {};
   iter.Start(topLevel,inst,temp,true);

   FUInstance* res = vGetInstanceByName_(iter,temp,argc,args);

   va_end(args);

   return res;
}

Edge* FindEdge(FUInstance* out,int outIndex,FUInstance* in,int inIndex,int delay){
   FUDeclaration* inDecl = in->declaration;
   FUDeclaration* outDecl = out->declaration;

   Assert(out->accel == in->accel);
   Assert(inIndex < inDecl->inputDelays.size);
   Assert(outIndex < outDecl->outputLatencies.size);

   Accelerator* accel = out->accel;

   FOREACH_LIST(edge,accel->edges){
      if(edge->units[0].inst == (ComplexFUInstance*) out &&
         edge->units[0].port == outIndex &&
         edge->units[1].inst == (ComplexFUInstance*) in &&
         edge->units[1].port == inIndex &&
         edge->delay == delay) {
         return edge;
      }
   }

   return nullptr;
}

Edge* ConnectUnitsGetEdge(FUInstance* out,int outIndex,FUInstance* in,int inIndex,int delay){
   Edge* edge = ConnectUnitsGetEdge(out,outIndex,in,inIndex,delay,nullptr);

   return edge;
}

void ConnectUnits(FUInstance* out,int outIndex,FUInstance* in,int inIndex,int delay){
   ConnectUnitsGetEdge(out,outIndex,in,inIndex,delay);
}

void ConnectUnitsIfNotConnected(FUInstance* out,int outIndex,FUInstance* in,int inIndex,int delay){
   ConnectUnitsIfNotConnectedGetEdge(out,outIndex,in,inIndex,delay);
}

Edge* ConnectUnitsIfNotConnectedGetEdge(FUInstance* out,int outIndex,FUInstance* in,int inIndex,int delay){
   Accelerator* accel = out->accel;

   FOREACH_LIST(edge,accel->edges){
      if(edge->units[0].inst == out && edge->units[0].port == outIndex &&
         edge->units[1].inst == in  && edge->units[1].port == inIndex &&
         delay == edge->delay){
         return edge;
      }
   }

   return ConnectUnitsGetEdge(out,outIndex,in,inIndex,delay);
}

Edge* ConnectUnits(PortInstance out,PortInstance in,int delay){
   return ConnectUnitsGetEdge(out.inst,out.port,in.inst,in.port,delay);
}

Edge* ConnectUnits(FUInstance* out,int outIndex,FUInstance* in,int inIndex,int delay,Edge* previousEdge){
   Edge* edge = ConnectUnitsGetEdge(out,outIndex,in,inIndex,delay,previousEdge);

   return edge;
}

FUDeclaration* RegisterFU(Versat* versat,FUDeclaration decl){
   if(decl.type == FUDeclaration::SINGLE || decl.type == FUDeclaration::SPECIAL){
      decl.totalOutputs = decl.outputLatencies.size;
   }

   FUDeclaration* type = versat->declarations.Alloc();
   *type = decl;

   return type;
}

// Returns the values when looking at a unit individually
UnitValues CalculateIndividualUnitValues(ComplexFUInstance* inst){
   UnitValues res = {};

   FUDeclaration* type = inst->declaration;

   res.outputs = type->outputLatencies.size; // Common all cases
   if(type->type == FUDeclaration::COMPOSITE){
      Assert(inst->declaration->fixedDelayCircuit);
   } else if(type->type == FUDeclaration::ITERATIVE){
      Assert(inst->declaration->fixedDelayCircuit);
      res.delays = 1;
      res.extraData = sizeof(int);
   } else {
      res.configs = type->configs.size;
      res.states = type->states.size;
      res.delays = type->nDelays;
      res.extraData = type->extraDataSize;
   }

   return res;
}

// Returns the values when looking at subunits as well. For simple units, same as individual unit values
// For composite units, same as calculate accelerator values + calculate individual unit values
UnitValues CalculateAcceleratorUnitValues(Versat* versat,ComplexFUInstance* inst){
   UnitValues res = {};

   FUDeclaration* type = inst->declaration;

   if(type->type == FUDeclaration::COMPOSITE){
      res.totalOutputs = type->totalOutputs;
   } else {
      res = CalculateIndividualUnitValues(inst);
      res.totalOutputs = res.outputs;
   }

   return res;
}

UnitValues CalculateAcceleratorValues(Versat* versat,Accelerator* accel){
   ArenaMarker marker(&accel->versat->temp);

   UnitValues val = {};

   std::vector<bool> seenShared;

   Hashmap<StaticId,int>* staticSeen = PushHashmap<StaticId,int>(&accel->versat->temp,1000);

   int memoryMappedDWords = 0;

   // Handle non-static information
   FOREACH_LIST(ptr,accel->allocated){
      ComplexFUInstance* inst = ptr->inst;
      FUDeclaration* type = inst->declaration;

      // Check if shared
      if(inst->sharedEnable){
         if(inst->sharedIndex >= (int) seenShared.size()){
            seenShared.resize(inst->sharedIndex + 1);
         }

         if(!seenShared[inst->sharedIndex]){
            val.configs += type->configs.size;
            val.sharedUnits += 1;
         }

         seenShared[inst->sharedIndex] = true;
      } else if(!inst->isStatic){ // Shared cannot be static
         val.configs += type->configs.size;
      }

      if(type->isMemoryMapped){
         val.isMemoryMapped = true;

         memoryMappedDWords = AlignBitBoundary(memoryMappedDWords,type->memoryMapBits);
         memoryMappedDWords += 1 << type->memoryMapBits;
      }

      if(type == BasicDeclaration::input){
         val.inputs += 1;
      }

      val.states += type->states.size;
      val.delays += type->nDelays;
      val.ios += type->nIOs;
      val.extraData += type->extraDataSize;

      if(type->externalMemory.size){
         val.externalMemoryInterfaces += type->externalMemory.size;

         for(ExternalMemoryInterface& inter : type->externalMemory){
            val.externalMemorySize += (1 << inter.bitsize);
         }
      }
   }

   val.memoryMappedBits = log2i(memoryMappedDWords);

   ComplexFUInstance* outputInstance = GetOutputInstance(accel->allocated);
   if(outputInstance){
      FOREACH_LIST(edge,accel->edges){
         if(edge->units[0].inst == outputInstance){
            val.outputs = std::max(val.outputs - 1,edge->units[0].port) + 1;
         }
         if(edge->units[1].inst == outputInstance){
            val.outputs = std::max(val.outputs - 1,edge->units[1].port) + 1;
         }
      }
   }
   val.totalOutputs = CalculateTotalOutputs(accel);

   // Handle static information
   FOREACH_LIST(ptr,accel->allocated){
      ComplexFUInstance* inst = ptr->inst;
      if(inst->isStatic){
         StaticId id = {};

         id.name = inst->name;

         staticSeen->Insert(id,1);
         val.statics += inst->declaration->configs.size;
      }

      if(inst->declaration->type == FUDeclaration::COMPOSITE){
         for(Pair<StaticId,StaticData> pair : inst->declaration->staticUnits){
            int* possibleFind = staticSeen->Get(pair.first);

            if(!possibleFind){
               staticSeen->Insert(pair.first,1);
               val.statics += pair.second.configs.size;
            }
         }
      }
   }

   #if 0
   // Huffman encoding for memory mapping bits
   int last = -1;
   while(1){
      for(int i = 0; i < 32; i++){
         if(memoryMapBits[i]){
            memoryMapBits[i+1] += (memoryMapBits[i] / 2);
            memoryMapBits[i] = memoryMapBits[i] % 2;
            last = i;
         }
      }

      int first = -1;
      int second = -1;
      for(int i = 0; i < 32; i++){
         if(first == -1 && memoryMapBits[i] == 1){
            first = i;
         } else if(second == -1 && memoryMapBits[i] == 1){
            second = i;
            break;
         }
      }

      if(second == -1){
         break;
      }

      memoryMapBits[first] = 0;
      memoryMapBits[second] = 0;
      memoryMapBits[std::max(first,second) + 1] += 1;
   }

   if(last != -1){
      val.isMemoryMapped = true;
      val.memoryMappedBits = last;
   }
   #endif

   return val;
}

FUDeclaration* RegisterSubUnit(Versat* versat,String name,Accelerator* circuit){
   FUDeclaration decl = {};

   Arena* permanent = &versat->permanent;
   Arena* temp = &versat->temp;
   ArenaMarker marker(temp);

   // Check if it's completely combinatorial
   bool combinatorial = true;
   FOREACH_LIST(ptr,circuit->allocated){
      FUDeclaration* decl = ptr->inst->declaration;
      if(decl->type != FUDeclaration::SPECIAL && !decl->isOperation){
         combinatorial = false;
      }
   }

   if(combinatorial){
      circuit = Flatten(versat,circuit,99);
   }

   decl.type = FUDeclaration::COMPOSITE;
   decl.name = name;

   // HACK, for now
   circuit->subtype = &decl;

   OutputGraphDotFile(versat,circuit,false,"debug/%.*s/base.dot",UNPACK_SS(name));
   decl.baseCircuit = CopyAccelerator(versat,circuit,nullptr);
   OutputGraphDotFile(versat,decl.baseCircuit,false,"debug/%.*s/base2.dot",UNPACK_SS(name));

   ReorganizeAccelerator(circuit,temp);

   region(temp){
      auto delays = CalculateDelay(versat,circuit,temp);

      OutputGraphDotFile(versat,circuit,false,"debug/%.*s/beforeFixDelay.dot",UNPACK_SS(name));

      FixDelays(versat,circuit,delays);

      OutputGraphDotFile(versat,circuit,false,"debug/%.*s/fixDelay.dot",UNPACK_SS(name));
      //DebugValue(MakeValue(delays,"Hashmap<EdgeNode,int>"),temp);
   }

   ReorganizeAccelerator(circuit,temp);
   decl.fixedDelayCircuit = circuit;

   FOREACH_LIST(ptr,circuit->allocated){
      if(ptr->multipleSamePortInputs){
         printf("Multiple inputs: %.*s\n",UNPACK_SS(name));
         break;
      }
   }

   // Need to calculate versat data here.
   UnitValues val = CalculateAcceleratorValues(versat,circuit);

   bool anySavedConfiguration = false;
   FOREACH_LIST(ptr,circuit->allocated){
      ComplexFUInstance* inst = ptr->inst;
      anySavedConfiguration |= inst->savedConfiguration;
   }

   decl.nDelays = val.delays;
   decl.nIOs = val.ios;
   decl.extraDataSize = val.extraData;
   decl.nStaticConfigs = val.statics;
   decl.isMemoryMapped = val.isMemoryMapped;
   decl.memoryMapBits = val.memoryMappedBits;
   decl.memAccessFunction = CompositeMemoryAccess;

   decl.inputDelays = PushArray<int>(permanent,val.inputs);

   decl.externalMemory = PushArray<ExternalMemoryInterface>(permanent,val.externalMemoryInterfaces);
   int externalIndex = 0;
   FOREACH_LIST(ptr,circuit->allocated){
      for(ExternalMemoryInterface& inter : ptr->inst->declaration->externalMemory){
         decl.externalMemory[externalIndex++] = inter;
      }
   }

   for(int i = 0; i < val.inputs; i++){
      ComplexFUInstance* input = GetInputInstance(circuit->allocated,i);

      if(!input){
         continue;
      }

      decl.inputDelays[i] = input->baseDelay;
   }

   decl.outputLatencies = PushArray<int>(permanent,val.outputs);

   ComplexFUInstance* outputInstance = GetOutputInstance(circuit->allocated);
   if(outputInstance){
      for(int i = 0; i < decl.outputLatencies.size; i++){
         decl.outputLatencies[i] = outputInstance->baseDelay;
      }
   }

   decl.configs = PushArray<Wire>(permanent,val.configs);
   decl.states = PushArray<Wire>(permanent,val.states);

   Hashmap<int,int>* staticsSeen = PushHashmap<int,int>(temp,val.sharedUnits);

   int configIndex = 0;
   int stateIndex = 0;

   FOREACH_LIST(ptr,circuit->allocated){
      ComplexFUInstance* inst = ptr->inst;
      FUDeclaration* d = inst->declaration;

      if(!inst->isStatic){
         if(inst->sharedEnable){
            if(staticsSeen->InsertIfNotExist(inst->sharedIndex,0)){
               for(Wire& wire : d->configs){
                  decl.configs[configIndex].name = PushString(permanent,"%.*s_%.2d",UNPACK_SS(wire.name),configIndex);
                  decl.configs[configIndex++].bitsize = wire.bitsize;
               }
            }
         } else {
            for(Wire& wire : d->configs){
               decl.configs[configIndex].name = PushString(permanent,"%.*s_%.2d",UNPACK_SS(wire.name),configIndex);
               decl.configs[configIndex++].bitsize = wire.bitsize;
            }
         }
      }

      for(Wire& wire : d->states){
         decl.states[stateIndex].name = PushString(permanent,"%.*s_%.2d",UNPACK_SS(wire.name),stateIndex);
         decl.states[stateIndex++].bitsize = wire.bitsize;
      }
   }

   if(anySavedConfiguration){
      decl.defaultConfig = PushArray<iptr>(permanent,val.configs);
      decl.defaultStatic = PushArray<iptr>(permanent,val.statics);

      int configIndex = 0;
      //int staticIndex = 0;
      bool didOnce = false;
      FOREACH_LIST(ptr,circuit->allocated){
         ComplexFUInstance* inst = ptr->inst;

         if(inst->savedConfiguration){
            if(inst->isStatic){
               Assert(!didOnce); // In order to properly do default config for statics, we need to first map their location and then copy
               didOnce = true;
               for(int i = 0; i < inst->declaration->configs.size; i++){
                  decl.defaultStatic[configIndex++] = inst->config[i];
               }
            } else {
               for(int i = 0; i < inst->declaration->configs.size; i++){
                  decl.defaultConfig[configIndex++] = inst->config[i];
               }
            }
         }
      }
   }

   decl.configOffsets = CalculateConfigurationOffset(circuit,MemType::CONFIG,permanent);
   decl.stateOffsets = CalculateConfigurationOffset(circuit,MemType::STATE,permanent);
   decl.delayOffsets = CalculateConfigurationOffset(circuit,MemType::DELAY,permanent);

   Assert(decl.configOffsets.max == val.configs);
   Assert(decl.stateOffsets.max == val.states);
   Assert(decl.delayOffsets.max == val.delays);

   decl.outputOffsets = CalculateOutputsOffset(circuit,val.outputs,permanent);
   Assert(decl.outputOffsets.max == val.totalOutputs + val.outputs);
   decl.totalOutputs = val.totalOutputs + val.outputs; // Includes the output portion of the composite instance

   decl.extraDataOffsets = CalculateConfigurationOffset(circuit,MemType::EXTRA,permanent);
   Assert(decl.extraDataOffsets.max == val.extraData);

   //Assert(extraDataIndex <= decl.extraDataSize);

   // TODO: Change unit delay type inference. Only care about delay type to upper levels.
   // Type source only if a source unit is connected to out. Type sink only if there is a input to sink connection
   bool hasSourceDelay = false;
   bool hasSinkDelay = false;
   bool implementsDone = false;

   FOREACH_LIST(ptr,circuit->allocated){
      ComplexFUInstance* inst = ptr->inst;
      if(inst->declaration->type == FUDeclaration::SPECIAL){
         continue;
      }

      if(inst->declaration->implementsDone){
         implementsDone = true;
      }
      if(ptr->type == InstanceNode::TAG_SINK){
         hasSinkDelay = CHECK_DELAY(inst,DELAY_TYPE_SINK_DELAY);
      }
      if(ptr->type == InstanceNode::TAG_SOURCE){
         hasSourceDelay = CHECK_DELAY(inst,DELAY_TYPE_SOURCE_DELAY);
      }
      if(ptr->type == InstanceNode::TAG_SOURCE_AND_SINK){
         hasSinkDelay = CHECK_DELAY(inst,DELAY_TYPE_SINK_DELAY);
         hasSourceDelay = CHECK_DELAY(inst,DELAY_TYPE_SOURCE_DELAY);
      }
   }

   if(hasSourceDelay){
      decl.delayType = (DelayType) ((int)decl.delayType | (int) DelayType::DELAY_TYPE_SOURCE_DELAY);
   }
   if (hasSinkDelay){
      decl.delayType = (DelayType) ((int)decl.delayType | (int) DelayType::DELAY_TYPE_SINK_DELAY);
   }

   decl.implementsDone = implementsDone;

   decl.staticUnits = PushHashmap<StaticId,StaticData>(permanent,1000); // TODO: Set correct number of elements
   int staticOffset = 0;
   // Start by collecting all the existing static allocated units in subinstances
   FOREACH_LIST(ptr,circuit->allocated){
      ComplexFUInstance* inst = ptr->inst;
      if(inst->declaration->type == FUDeclaration::COMPOSITE ||
         inst->declaration->type == FUDeclaration::ITERATIVE){

         for(auto pair : inst->declaration->staticUnits){
            StaticData newData = pair.second;
            newData.offset = staticOffset;

            if(decl.staticUnits->InsertIfNotExist(pair.first,newData)){
               staticOffset += newData.configs.size;
            }
         }
      }
   }

   FUDeclaration* res = RegisterFU(versat,decl);
   res->baseCircuit->subtype = res;
   res->fixedDelayCircuit->subtype = res;

   // Add this units static instances (needs be done after Registering the declaration because the parent is a pointer to the declaration)
   FOREACH_LIST(ptr,circuit->allocated){
      ComplexFUInstance* inst = ptr->inst;
      if(inst->isStatic){
         StaticId id = {};
         id.name = inst->name;
         id.parent = res;

         StaticData data = {};
         data.configs = inst->declaration->configs;
         data.offset = staticOffset;

         if(res->staticUnits->InsertIfNotExist(id,data)){
            staticOffset += inst->declaration->configs.size;
         }
      }
   }

   if(versat->debug.outputAccelerator){
      char buffer[256];
      sprintf(buffer,"src/%.*s.v",UNPACK_SS(decl.name));
      FILE* sourceCode = OpenFileAndCreateDirectories(buffer,"w");
      OutputCircuitSource(versat,res,circuit,sourceCode);
      fclose(sourceCode);
   }

   return res;
}

static int* IterativeInitializeFunction(ComplexFUInstance* inst){
   return nullptr;
}

static int* IterativeStartFunction(ComplexFUInstance* inst){
   int* delay = (int*) inst->extraData;

   *delay = 0;

   return nullptr;
}

static int* IterativeUpdateFunction(ComplexFUInstance* inst,Array<int> inputs){
   static int out[99];

   FUDeclaration* type = inst->declaration;
   int* delay = (int*) inst->extraData;
   ComplexFUInstance* dataInst = (ComplexFUInstance*) GetInstanceByName(inst->iterative,"data");

   //LockAccelerator(inst->compositeAccel,Accelerator::Locked::ORDERED);
   //Assert(inst->iterative);

   if(*delay == -1){ // For loop part
      AcceleratorIterator iter = {};
      Arena* arena = &inst->accel->versat->temp;
      BLOCK_REGION(arena);
      iter.Start(inst->accel,arena,true);
      AcceleratorRunComposite(iter);
   } else if(*delay == inst->delay[0]){
      for(int i = 0; i < type->inputDelays.size; i++){
         int val = inputs[i]; //GetInputValue(inst,i);
         SetInputValue(inst->iterative,i,val);
      }

      AcceleratorRun(inst->iterative);

      for(int i = 0; i < type->outputLatencies.size; i++){
         int val = GetOutputValue(inst->iterative,i);
         out[i] = val;
      }

      ComplexFUInstance* secondData = (ComplexFUInstance*) GetInstanceByName(inst->iterative,"data");

      for(int i = 0; i < secondData->declaration->outputLatencies.size; i++){
         dataInst->outputs[i] = secondData->outputs[i];
         dataInst->storedOutputs[i] = secondData->outputs[i];
      }

      *delay = -1;
   } else {
      *delay += 1;
   }

   return out;
}

static int* IterativeDestroyFunction(ComplexFUInstance* inst){
   return nullptr;
}

FUDeclaration* RegisterIterativeUnit(Versat* versat,IterativeUnitDeclaration* decl){
   FUInstance* comb = nullptr;
   FOREACH_LIST(node,decl->forLoop->allocated){
      if(node->inst->declaration == decl->baseDeclaration){
         comb = node->inst;
      }
   }

   FUDeclaration* combType = comb->declaration;
   FUDeclaration declaration = {};

   // Default combinatorial values
   declaration = *combType; // By default, copy everything from combinatorial declaration
   declaration.type = FUDeclaration::ITERATIVE;
   declaration.staticUnits = combType->staticUnits;
   declaration.nDelays = 1; // At least one delay

   // Accelerator computed values
   UnitValues val = CalculateAcceleratorValues(versat,decl->forLoop);
   declaration.extraDataSize = val.extraData + sizeof(int); // Save delay

   // Values from iterative declaration
   declaration.name = decl->name;
   declaration.unitName = decl->unitName;
   declaration.initial = decl->initial;
   declaration.forLoop = decl->forLoop;
   declaration.dataSize = decl->dataSize;

   declaration.initializeFunction = IterativeInitializeFunction;
   declaration.startFunction = IterativeStartFunction;
   declaration.updateFunction = IterativeUpdateFunction;
   declaration.destroyFunction = IterativeDestroyFunction;
   declaration.memAccessFunction = CompositeMemoryAccess;

   declaration.inputDelays = PushArray<int>(&versat->permanent,val.inputs);
   declaration.outputLatencies = PushArray<int>(&versat->permanent,val.outputs);
   Memset(declaration.inputDelays,0);
   Memset(declaration.outputLatencies,decl->latency);

   FUDeclaration* registeredType = RegisterFU(versat,declaration);

   if(!versat->debug.outputAccelerator){
      return registeredType;
   }

   char buffer[256];
   sprintf(buffer,"src/%.*s.v",UNPACK_SS(decl->name));
   FILE* sourceCode = OpenFileAndCreateDirectories(buffer,"w");

   TemplateSetCustom("base",registeredType,"FUDeclaration");
   TemplateSetCustom("comb",comb,"ComplexFUInstance");

   #if 0
   FUInstance* firstPartComb = nullptr;
   FUInstance* firstData = nullptr;
   FUInstance* secondPartComb = nullptr;
   FUInstance* secondData = nullptr;

   for(FUInstance* inst : decl->initial->instances){
      if(inst->declaration == decl->baseDeclaration){
         firstPartComb = inst;
      }
      if(inst->declaration == BasicDeclaration::data){
         firstData = inst;
      }
   }
   for(FUInstance* inst : decl->forLoop->instances){
      if(inst->declaration == decl->baseDeclaration){
         secondPartComb = inst;
      }
      if(inst->declaration == BasicDeclaration::data){
         secondData = inst;
      }
   }

   TemplateSetCustom("versat",versat,"Versat");
   TemplateSetCustom("firstComb",firstPartComb,"ComplexFUInstance");
   TemplateSetCustom("secondComb",secondPartComb,"ComplexFUInstance");
   TemplateSetCustom("firstOut",decl->initial->outputInstance,"ComplexFUInstance");
   TemplateSetCustom("secondOut",decl->forLoop->outputInstance,"ComplexFUInstance");
   TemplateSetCustom("firstData",firstData,"ComplexFUInstance");
   TemplateSetCustom("secondData",secondData,"ComplexFUInstance");
   #endif

   //ProcessTemplate(sourceCode,"../../submodules/VERSAT/software/templates/versat_iterative_template.tpl",&versat->temp);

   fclose(sourceCode);

   return registeredType;
}

void ClearConfigurations(Accelerator* accel){
   for(int i = 0; i < accel->configAlloc.size; i++){
      accel->configAlloc.ptr[i] = 0;
   }
}

void LoadConfiguration(Accelerator* accel,int configuration){
   // Implements the reverse of Save Configuration
}

void SaveConfiguration(Accelerator* accel,int configuration){
   //Assert(configuration < accel->versat->numberConfigurations);
}

void OutputMemoryMap(Versat* versat,Accelerator* accel){
   UNHANDLED_ERROR; // Might need to create a view so that ComputeVersatValues works
   VersatComputedValues val = ComputeVersatValues(versat,accel);

   printf("\n");
   printf("Total bytes mapped: %d\n",val.memoryMappedBytes);
   printf("Maximum bytes mapped by a unit: %d\n",val.maxMemoryMapDWords * 4);
   printf("Memory address bits: %d\n",val.memoryAddressBits);
   printf("Units mapped: %d\n",val.unitsMapped);
   printf("Memory mapping address bits: %d\n",val.memoryMappingAddressBits);
   printf("\n");
   printf("Config registers: %d\n",val.nConfigs);
   printf("Config bits used: %d\n",val.configBits);
   printf("\n");
   printf("Static registers: %d\n",val.nStatics);
   printf("Static bits used: %d\n",val.staticBits);
   printf("\n");
   printf("Delay registers: %d\n",val.nDelays);
   printf("Delay bits used: %d\n",val.delayBits);
   printf("\n");
   printf("Configuration registers: %d (including versat reg, static and delays)\n",val.nConfigurations);
   printf("Configuration address bits: %d\n",val.configurationAddressBits);
   printf("Configuration bits used: %d\n",val.configurationBits);
   printf("\n");
   printf("State registers: %d (including versat reg)\n",val.nStates);
   printf("State address bits: %d\n",val.stateAddressBits);
   printf("State bits used: %d\n",val.stateBits);
   printf("\n");
   printf("IO connections: %d\n",val.nUnitsIO);

   printf("\n");
   printf("Number units: %d\n",Size(versat->accelerators.Get(0)->allocated));
   printf("\n");

   #define ALIGN_FORMAT "%-14s"
   #define ALIGN_SIZE 14

   int bitsNeeded = val.lowerAddressSize;

   printf(ALIGN_FORMAT,"Address:");
   for(int i = bitsNeeded; i >= 10; i--)
      printf("%d ",i/10);
   printf("\n");
   printf(ALIGN_FORMAT," ");
   for(int i = bitsNeeded; i >= 0; i--)
      printf("%d ",i%10);
   printf("\n");
   for(int i = bitsNeeded + (ALIGN_SIZE / 2); i >= 0; i--)
      printf("==");
   printf("\n");

   // Memory mapped
   printf(ALIGN_FORMAT,"MemoryMapped:");
   printf("1 ");
   for(int i = bitsNeeded - 1; i >= 0; i--)
      if(i < val.memoryAddressBits)
         printf("M ");
      else
         printf("0 ");
   printf("\n");
   for(int i = bitsNeeded + (ALIGN_SIZE / 2); i >= 0; i--)
      printf("==");
   printf("\n");

   // Versat registers
   printf(ALIGN_FORMAT,"Versat Regs:");
   for(int i = bitsNeeded - 0; i >= 0; i--)
      printf("0 ");
   printf("\n");
   for(int i = bitsNeeded + (ALIGN_SIZE / 2); i >= 0; i--)
      printf("==");
   printf("\n");

   // Config/State
   printf(ALIGN_FORMAT,"Config/State:");
   printf("0 ");
   for(int i = bitsNeeded - 1; i >= 0; i--){
      if(i < val.configurationAddressBits && i < val.stateAddressBits)
         printf("B ");
      else if(i < val.configurationAddressBits)
         printf("C ");
      else if(i < val.stateAddressBits)
         printf("S ");
      else
         printf("0 ");
   }
   printf("\n");
   for(int i = bitsNeeded + (ALIGN_SIZE / 2); i >= 0; i--)
      printf("==");
   printf("\n");

   printf("\n");
   printf("M - Memory mapped\n");
   printf("C - Used only by Config\n");
   printf("S - Used only by State\n");
   printf("B - Used by both Config and State\n");
   printf("\n");
   printf("Memory/Config bit: %d\n",val.memoryConfigDecisionBit);
   printf("Memory range: [%d:0]\n",val.memoryAddressBits - 1);
   printf("Config range: [%d:0]\n",val.configurationAddressBits - 1);
   printf("State range: [%d:0]\n",val.stateAddressBits - 1);
}

int GetInputValue(InstanceNode* node,int index){
   if(!node->inputs.size){
      return 0;
   }

   PortNode* other = &node->inputs[index];

   if(!other->node){
      return 0;
   }

   return other->node->inst->outputs[other->port];
}

int GetInputValue(FUInstance* inst,int index){
   InstanceNode* node = GetInstanceNode(inst->accel,(ComplexFUInstance*) inst);
   if(!node){
      return 0;
   }

   PortNode* other = &node->inputs[index];

   #if 01
   if(!other->node){
      return 0;
   }

   return other->node->inst->outputs[other->port];
   #endif
}

int GetNumberOfInputs(FUInstance* inst){
   return inst->declaration->inputDelays.size;
}

int GetNumberOfOutputs(FUInstance* inst){
   return inst->declaration->outputLatencies.size;
}

FUInstance* CreateOrGetInput(Accelerator* accel,String name,int portNumber){
   FOREACH_LIST(ptr,accel->allocated){
      ComplexFUInstance* inst = ptr->inst;
      if(inst->declaration == BasicDeclaration::input && inst->portIndex == portNumber){
         return inst;
      }
   }

   ComplexFUInstance* inst = (ComplexFUInstance*) CreateFUInstance(accel,BasicDeclaration::input,name);
   inst->portIndex = portNumber;

   return inst;
}

int GetNumberOfInputs(Accelerator* accel){
   int count = 0;
   FOREACH_LIST(ptr,accel->allocated){
      ComplexFUInstance* inst = ptr->inst;
      if(inst->declaration == BasicDeclaration::input){
         count += 1;
      }
   }

   return count;
}

int GetNumberOfOutputs(Accelerator* accel){
   NOT_IMPLEMENTED;
   return 0;
}

void SetInputValue(Accelerator* accel,int portNumber,int number){
   Assert(accel->outputAlloc.ptr);

   ComplexFUInstance* inst = GetInputInstance(accel->allocated,portNumber);
   Assert(inst);

   inst->outputs[0] = number;
   inst->storedOutputs[0] = number;
}

int GetOutputValue(Accelerator* accel,int portNumber){
   ComplexFUInstance* inst = GetOutputInstance(accel->allocated);

   int value = GetInputValue(inst,portNumber);

   return value;
}

int GetInputPortNumber(FUInstance* inputInstance){
   Assert(inputInstance->declaration == BasicDeclaration::input);

   return ((ComplexFUInstance*) inputInstance)->portIndex;
}

void SetDelayRecursive_(AcceleratorIterator iter,int delay){
   ComplexFUInstance* inst = iter.Current()->inst;

   if(inst->declaration == BasicDeclaration::buffer){
      inst->config[0] = inst->bufferAmount;
      return;
   }

   int totalDelay = inst->baseDelay + delay;

   if(inst->declaration->type == FUDeclaration::COMPOSITE){
      AcceleratorIterator it = iter.LevelBelowIterator();

      for(InstanceNode* child = it.Current(); child; child = it.Skip()){
         SetDelayRecursive_(it,totalDelay);
      }
   } else if(inst->declaration->nDelays){
      inst->delay[0] = totalDelay;
   }
}

int CountNonSpecialChilds(InstanceNode* nodes){
   int count = 0;
   FOREACH_LIST(ptr,nodes){
      ComplexFUInstance* inst = ptr->inst;
      if(inst->declaration->type == FUDeclaration::SPECIAL){
      } else if(inst->declaration->type == FUDeclaration::COMPOSITE){
         count += CountNonSpecialChilds(inst->declaration->baseCircuit->allocated);
      } else {
         count += 1;
      }
   }

   return count;
}

void SetDelayRecursive(Accelerator* accel,Arena* arena){
   BLOCK_REGION(arena);

   AcceleratorIterator iter = {};
   for(InstanceNode* node = iter.Start(accel,arena,true); node; node = iter.Skip()){
      SetDelayRecursive_(iter,0);
   }
}

bool CheckValidName(String name){
   for(int i = 0; i < name.size; i++){
      char ch = name[i];

      bool allowed = (ch >= 'a' && ch <= 'z')
                 ||  (ch >= 'A' && ch <= 'Z')
                 ||  (ch >= '0' && ch <= '9' && i != 0)
                 ||  (ch == '_')
                 ||  (ch == '.' && i != 0)  // For now allow it, despite the fact that it should only be used internally by Versat
                 ||  (ch == '/' && i != 0); // For now allow it, despite the fact that it should only be used internally by Versat

      if(!allowed){
         return false;
      }
   }

   return true;
}

int CalculateMemoryUsage(Versat* versat){
   int totalSize = 0;
   #if 0
   for(Accelerator* accel : versat->accelerators){
      int accelSize = accel->instances.MemoryUsage() + accel->edges.MemoryUsage();
      accelSize += MemoryUsage(accel->configAlloc);
      accelSize += MemoryUsage(accel->stateAlloc);
      accelSize += MemoryUsage(accel->delayAlloc);
      accelSize += MemoryUsage(accel->staticAlloc);
      accelSize += MemoryUsage(accel->outputAlloc);
      accelSize += MemoryUsage(accel->storedOutputAlloc);
      accelSize += MemoryUsage(accel->extraDataAlloc);

      totalSize += accelSize;
   }

   totalSize += versat->permanent.used; // Not total allocated, because it would distort the actual data we care about
   #endif

   return totalSize;
}

#include "debugGUI.hpp"

void Hook(Versat* versat,FUDeclaration* decl,Accelerator* accel,FUInstance* inst){

}




