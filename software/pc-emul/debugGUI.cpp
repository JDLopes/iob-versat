#include "debugGUI.hpp"

//#define DEBUG_GUI

#ifdef DEBUG_GUI
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#include <SDL.h>
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <SDL_opengles2.h>
#else
#include <SDL_opengl.h>
#endif
#include "imnodes.h"

#include "utils.hpp"
#include "type.hpp"
#include "textualRepresentation.hpp"
#include "configurations.hpp"

// Simulate c++23 feature
template<typename T>
Optional<T> OrElse(Optional<T> first,Optional<T> elseOpt){
   if(first){
      return first;
   } else {
      return elseOpt;
   }
}

SDL_Window* InitWindow(){
   static SDL_Window* window = nullptr;

   if(window){
      return window;
   }

   if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0){
      printf("Error: %s\n", SDL_GetError());
      return nullptr;
   }

   // GL 3.0 + GLSL 130
   const char* glsl_version = "#version 130";
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

   // Create window with graphics context
   SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
   SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
   SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
   SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
   window = SDL_CreateWindow("Debug View", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);

   SDL_GLContext gl_context = SDL_GL_CreateContext(window);
   SDL_GL_MakeCurrent(window, gl_context);
   SDL_GL_SetSwapInterval(1); // Enable vsync

   // Setup Dear ImGui context
   IMGUI_CHECKVERSION();
   ImGui::CreateContext();

   ImNodes::CreateContext();
   ImNodes::SetNodeGridSpacePos(1, ImVec2(200.0f, 200.0f));

   // Setup Dear ImGui style
   ImGui::StyleColorsDark();

   // Setup Platform/Renderer backends
   ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
   ImGui_ImplOpenGL3_Init(glsl_version);

   return window;

#if 0
   // Cleanup
   ImGui_ImplOpenGL3_Shutdown();
   ImGui_ImplSDL2_Shutdown();
   ImGui::DestroyContext();

   SDL_GL_DeleteContext(gl_context);
   SDL_DestroyWindow(window);
   SDL_Quit();
#endif // 0
}

#if 0
struct GraphNode{
   ComplexFUInstance* inst;
   ImVec2 pos;
   int order;
};

struct GraphEdge{
   int output;
   int input;

};

void DrawNode(ImDrawList* draw,GraphNode* node,ImVec2 canvas){
   ImVec2 pos = node->pos;
   pos.x += canvas.x;
   pos.y += canvas.y;

   ImU32 col = IM_COL32(255,0,0,255);
   draw->AddCircleFilled(pos,20.0f,col);

   char* text = "here";
   pos.x -= 13.0f;
   pos.y -= 7.0f;
   ImU32 black = IM_COL32(0,0,0,255);
   draw->AddText(pos,black,text,text+4);
}

void DisplayAccelerator(Accelerator* accel,Arena* arena){
   #if 0
   BLOCK_REGION(arena);

   int size = Size(accel->allocated);
   Array<GraphNode> nodes = PushArray<GraphNode>(arena,size);
   Memset(nodes,{});

   #if 0
   FOREACH(ptr,accel->instances){
      nodes
   }
   #endif

   float yPos = 0.0f;
   float lastOrder = 0;
   AcceleratorView view = CreateAcceleratorView(accel,arena);
   DAGOrder order = view.CalculateDAGOrdering(arena);
   for(int i = 0; i < Size(accel->allocated); i++){
      int ord = order.instances[i]->graphData->order;

      if(ord != lastOrder){
         lastOrder = ord;
         yPos = 0.0f;
      }

      nodes[i].inst = order.instances[i];
      nodes[i].order = ord;
      nodes[i].pos.x = ord * 100.0f + 50.0f;
      nodes[i].pos.y = yPos + 50.0f;
      yPos += 50.0f;
   }

   //ImGui::Text("here");
   if(ImGui::BeginChild("Canvas", ImVec2(0.f, 0.f), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollWithMouse)){
      ImVec2 canvas_p0 = ImGui::GetCursorScreenPos();

      ImVec2 canvas_sz = ImGui::GetContentRegionAvail();
      ImGui::InvisibleButton("canvas", canvas_sz, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);

      ImDrawList* draw_list = ImGui::GetWindowDrawList();
      for(int i = 0; i < nodes.size; i++){
         DrawNode(draw_list,&nodes[i],canvas_p0);
      }
   }
   ImGui::EndChild();
   #endif
}
#endif

struct FilterConfigurations{
   bool showConfig;
   bool showStatic;
   bool showState;
   bool showDelay;
   bool showMem;
   bool showOutputs;
   bool showExtraData;
};

struct FilterTypes{
   bool showSimple;
   bool showNoOperation;
   bool showComposite;
   bool showSpecial;
};

struct Filter{
   FilterConfigurations config;
   FilterTypes types;

   bool doAnd;
};

struct GraphNode{
   InstanceNode* node;

   enum {SINGLE,CONNECTION} type;
};

void AcceleratorGraph(Accelerator* accel,Arena* arena,Filter filter){
#if 0
   BLOCK_REGION(arena);

   DAGOrderNodes order = CalculateDAGOrder(accel->allocated,arena);

   int size = accel->numberInstances;
   int maxOrder = order.maxOrder;

   Array<Array<GraphNode>> ordered = PushArray<Array<GraphNode>>(arena,maxOrder);
   Memset(ordered,{});

   int orderIndex = 0;
   int index = 0;
   for(int orderIndex = 0; orderIndex < maxOrder; orderIndex += 1){
      int inserted = 0;
      Byte* mark = MarkArena(arena);
      for(; index < size; index += 1){
         if(order.order[index] != orderIndex){
            break;
         }
         GraphNode* node = PushStruct<GraphNode>(arena);
         node->node = order.instances[index];
      }
      ordered[orderIndex] = PointArray<GraphNode>(arena,mark);
   }

   if(ImGui::Begin("AcceleratorGraph")){
      ImNodes::BeginNodeEditor();

      int nodes = 0;
      for(int order = 0; order < maxOrder; order++){
         for(int i = 0; i < ordered[order].size; i++){
            ImNodes::SetNodeGridSpacePos(nodes,ImVec2(order * 200.0f,i * 200.0f));
            ImNodes::BeginNode(nodes++);
            ImGui::Dummy(ImVec2(20.0f,20.0f));
            ImNodes::EndNode();
         }
      }

      #if 0
      ImNodes::BeginNode(0);
      ImNodes::BeginNodeTitleBar();
      ImGui::TextUnformatted("Lol");
      ImNodes::EndNodeTitleBar();
      ImNodes::BeginOutputAttribute(0);
      ImGui::Text("output pin");
      ImNodes::EndOutputAttribute();
      ImNodes::EndNode();

      ImNodes::BeginNode(1);
      ImNodes::BeginNodeTitleBar();
      ImGui::TextUnformatted("Lol2");
      ImNodes::EndNodeTitleBar();
      ImNodes::BeginInputAttribute(1);
      ImGui::Text("input pin");
      ImNodes::EndOutputAttribute();

      ImNodes::EndNode();

      ImNodes::Link(0, 0, 1);
      #endif

      ImNodes::EndNodeEditor();
   }
   ImGui::End();
#endif
}

bool PassFilterConfiguration(Accelerator* topLevel,ComplexFUInstance* inst,FilterConfigurations filter){
   bool res = false;
   res |= filter.showConfig && inst->declaration->configs.size && !IsConfigStatic(topLevel,inst);
   res |= filter.showStatic && inst->declaration->configs.size && IsConfigStatic(topLevel,inst);
   res |= filter.showState && inst->declaration->states.size;
   res |= filter.showDelay && inst->declaration->nDelays;
   res |= filter.showMem && inst->declaration->isMemoryMapped;
   res |= filter.showOutputs && inst->declaration->outputLatencies.size;
   res |= filter.showExtraData && inst->declaration->extraDataSize;

   return res;
}

bool PassFilterTypes(ComplexFUInstance* inst,FilterTypes filter){
   bool res = false;

   bool simple = filter.showSimple && inst->declaration->type == FUDeclaration::SINGLE;
   bool operation = filter.showNoOperation && inst->declaration->isOperation;

   if(filter.showNoOperation){
      res |= (simple && !operation);
   } else {
      res |= simple;
   }

   res |= filter.showComposite && inst->declaration->type == FUDeclaration::COMPOSITE;
   res |= filter.showSpecial && inst->declaration->type == FUDeclaration::SPECIAL;

   return res;
}

bool PassFilter(Accelerator* topLevel,ComplexFUInstance* inst,Filter filter){
   bool configuration = PassFilterConfiguration(topLevel,inst,filter.config);
   bool type = PassFilterTypes(inst,filter.types);

   bool res = false;
   if(filter.doAnd){
      res = configuration & type;
   } else {
      res = configuration | type;
   }

   return res;
}

Optional<String> AcceleratorTreeNodes(AcceleratorIterator iter,Filter filter){
   static Byte nameBuffer[256];
   Arena nameArena = {};
   nameArena.totalAllocated = 256;
   nameArena.mem = nameBuffer;

   Optional<String> res = {};

   for(InstanceNode* node = iter.Current(); node; node = iter.Skip()){
      ComplexFUInstance* inst = node->inst;
      bool doNode = PassFilter(iter.topLevel,inst,filter);

      if(inst->declaration->type == FUDeclaration::COMPOSITE){
         int flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_Framed;
         if(ImGui::TreeNodeEx(inst,flags,"%.*s",UNPACK_SS(inst->name))){
            if(ImGui::IsItemClicked()){
               nameArena.used = 0;
               res = iter.GetFullName(&nameArena);
               //res = *inst;
            }

            Optional<String> val = AcceleratorTreeNodes(iter.LevelBelowIterator(),filter);

            if(val){
               Assert(!res);
               res = val;

               //res = val;
            }

            ImGui::TreePop();
         } else if(ImGui::IsItemClicked()){
            nameArena.used = 0;
            res = iter.GetFullName(&nameArena);
            //res = *inst;
         }
      } else {
         bool selected = false;
         if(doNode){
            ImGui::Selectable(StaticFormat("%.*s",UNPACK_SS(inst->name)),&selected);
         }

         if(selected){
            nameArena.used = 0;
            res = iter.GetFullName(&nameArena);
            //res = *inst;
         }
      }
   }

   return res;
}

String DebugValueRepresentation(Value val,Arena* arena){
   Type* FUDeclType = GetType(STRING("FUDeclaration"));
   Type* ComplexInstanceType = GetType(STRING("ComplexFUInstance"));
   Type* WireType = GetType(STRING("Wire"));
   //Type* SimpleInstanceType = GetType(STRING("SimpleFUInstance"));
   Type* EdgeNodeType = GetType(STRING("EdgeNode"));

   String repr = {};

   if(IsBasicType(val.type)){
      repr = GetDefaultValueRepresentation(val,arena);
      return repr;
   }

   Value collapsed = CollapsePtrIntoStruct(val);
   Type* type = collapsed.type;

   if(type == FUDeclType){
      auto decl = (FUDeclaration*) collapsed.custom;
      repr = Repr(decl,arena);
   } else if(type == ComplexInstanceType){
      auto inst = (ComplexFUInstance*) collapsed.custom;
      repr = inst->name;
   } else if(type == WireType){
      auto wire = (Wire*) collapsed.custom;
      repr = PushString(arena,"%.*s(%d)",UNPACK_SS(wire->name),wire->bitsize);
   } else if(type == EdgeNodeType){
      EdgeNode* node = (EdgeNode*) collapsed.custom;
      repr = Repr(*node,arena);
   } else {
      repr = GetDefaultValueRepresentation(val,arena);
   }

   return repr;
}

struct ValueSelected{
   Value val;
   int index;
};

Optional<ValueSelected> ShowStructTable(Value value,Arena* arena){
   Type* type = value.type;
   Assert(IsStruct(type));

   Optional<ValueSelected> res = {};
   if(ImGui::BeginTable("split",2,ImGuiTableFlags_RowBg)){
      int index = 0;
      for(Member& member : type->members){
         BLOCK_REGION(arena);

         Value memberVal = AccessStruct(value,&member);
         String repr = DebugValueRepresentation(memberVal,arena);

         bool selected = false;
         ImGui::TableNextRow();
         ImGui::TableNextColumn(); ImGui::Selectable(StaticFormat("%.*s",UNPACK_SS(member.name)),&selected,ImGuiSelectableFlags_SpanAllColumns);
         ImGui::TableNextColumn(); ImGui::Text("%.*s",UNPACK_SS(repr));

         if(selected){
            res = ValueSelected{memberVal,index};
         }
         index += 1;
      }
      ImGui::EndTable();
   }
   return res;
}

Optional<ValueSelected> ShowTable(Value iterable,Arena* arena,int selectedIndex = -1){
   BLOCK_REGION(arena);

   Optional<ValueSelected> res = {};
   if(ImGui::BeginTable("split",2,ImGuiTableFlags_RowBg)){
      Iterator iter = Iterate(iterable);
      int index = 0;
      for(Value val = GetValue(iter); HasNext(iter); Advance(&iter),index += 1){
         BLOCK_REGION(arena);
         String repr = DebugValueRepresentation(val,arena);

         bool selected = (index == selectedIndex);
         ImGui::TableNextRow();
         ImGui::TableNextColumn(); ImGui::Text("%d",index);
         ImGui::TableNextColumn(); selected = ImGui::Selectable(StaticFormat("%.*s##%d",UNPACK_SS(repr),index),&selected,ImGuiSelectableFlags_SpanAllColumns);

         if(selected){
            res = ValueSelected{val,index};
         }
      }
      ImGui::EndTable();
   }

   return res;
}

void ShowDualTable(Value iterable1,Value iterable2,Arena* arena){
   BLOCK_REGION(arena);

   if(ImGui::BeginTable("split",2,ImGuiTableFlags_RowBg)){
      Iterator iter1 = Iterate(iterable1);
      Iterator iter2 = Iterate(iterable2);
      int index = 0;
      for(; HasNext(iter1) && HasNext(iter2); Advance(&iter1),Advance(&iter2),index += 1){
         BLOCK_REGION(arena);
         String repr1 = DebugValueRepresentation(GetValue(iter1),arena);
         String repr2 = DebugValueRepresentation(GetValue(iter2),arena);

         ImGui::TableNextRow();
         ImGui::TableNextColumn(); ImGui::Text("[%d] %.*s",index,UNPACK_SS(repr1));
         ImGui::TableNextColumn(); ImGui::Text("%.*s",UNPACK_SS(repr2));
      }
      ImGui::EndTable();
   }
}

void ShowHashmapTable(Value hashmap,Arena* arena){
   BLOCK_REGION(arena);

   if(ImGui::BeginTable("split",2,ImGuiTableFlags_RowBg)){
      Iterator iter = Iterate(hashmap);
      int index = 0;
      for(; HasNext(iter); Advance(&iter),index += 1){
         BLOCK_REGION(arena);
         Value pair = GetValue(iter);

         Value val1 = AccessStruct(pair,STRING("first"));
         Value val2 = AccessStruct(pair,STRING("second"));

         String repr1 = DebugValueRepresentation(val1,arena);
         String repr2 = DebugValueRepresentation(val2,arena);

         ImGui::TableNextRow();
         ImGui::TableNextColumn(); ImGui::Text("[%d] %.*s",index,UNPACK_SS(repr1));
         ImGui::TableNextColumn(); ImGui::Text("%.*s",UNPACK_SS(repr2));
      }
      ImGui::EndTable();
   }
}

Optional<ValueSelected> OutputDebugValues(Value value,Arena* arena){
   Value collapsed = CollapsePtrIntoStruct(value);

   Type* type = collapsed.type;

   Optional<ValueSelected> res = {};
   if(type->type == Type::TEMPLATED_INSTANCE && type->templateBase == ValueType::HASHMAP){
      ShowHashmapTable(collapsed,arena);
   } else if(IsIndexable(type)){
      res = ShowTable(collapsed,arena);
   } else if(IsStruct(type)){
      res = ShowStructTable(collapsed,arena);
   }

   return res;
}

struct ValueSelection{
   Value val;
   int index;

   enum {SIMPLE,EMBEDDED_LIST} type;
};

static const int WINDOW_STATE_FRAME_SIZE = 10;
static const int WINDOW_STATE_INVALID_INDEX = -1;
struct ValueWindowState{
   const char* windowName;
   Optional<Value> startValue;
   int frame[WINDOW_STATE_FRAME_SIZE];
   bool embbededListStart;
   bool init;
};

void ClearWindowState(ValueWindowState* state,int startFrame){
   for(int i = startFrame; i < WINDOW_STATE_FRAME_SIZE; i++){
      state->frame[i] = WINDOW_STATE_INVALID_INDEX;
   }
}

void AddToWindowState(ValueWindowState* state,Value val){
   ClearWindowState(state,0);

   Value collapsed = CollapsePtrIntoStruct(val);

   Type* type = collapsed.type;
   if(IsEmbeddedListKind(type)){
      state->embbededListStart = true;
   }
   state->startValue = val;
   state->init = true;
}

/*

void UpdateWindowState(ValueWindowState* state){
   for(int i = 0; i < WINDOW_STATE_FRAME_SIZE; i++){
      if(!state->frame[i]){
         break;
      }

      ValueSelection val = state->frame[i].value();

      if(val.index == WINDOW_STATE_INVALID_INDEX){
         break; // Nothing else to do
      }

      if(val.type == ValueSelection::EMBEDDED_LIST){
         if(i + 1 >= WINDOW_STATE_FRAME_SIZE){
            break;
         }

         Iterator iter = Iterate(val.val);
         for(int index = 0; index < val.index; index++){
            Advance(&iter);
         }

         Value subValue = GetValue(iter);
         Value collapsed = CollapsePtrIntoStruct(subValue);

         if(state->frame[i + 1]){
            state->frame[i + 1].value().val = collapsed;
         } else {
            state->frame[i + 1] = ValueSelection{collapsed,WINDOW_STATE_INVALID_INDEX};
         }
      } else {
         Value subValue = AccessStruct(val.val,val.index);
         Value collapsed = CollapsePtrIntoStruct(subValue);
         if(state->frame[i + 1]){
            state->frame[i + 1].value().val = collapsed;
         } else {
            AddToWindowState(collapsed,state,i + 1,WINDOW_STATE_INVALID_INDEX);
         }
      }
   }
}

void AddToWindowState(Value val,ValueWindowState* state,int frame,int index){
   if(frame >= WINDOW_STATE_FRAME_SIZE){
      return;
   }

   Value collapsed = CollapsePtrIntoStruct(val);

   if(!state->frame[frame]){
      ClearWindowState(state,frame);
      state->frame[frame] = ValueSelection{collapsed,index};
      if(IsEmbeddedListKind(collapsed.type)){
         state->frame[frame].value().type = ValueSelection::EMBEDDED_LIST;
      }

      UpdateWindowState(state);
      return;
   }

   Type* oldType = state->frame[frame].value().val.type;
   Type* newType = collapsed.type;

   if(oldType != newType){
      ClearWindowState(state,frame);
      state->frame[frame] = ValueSelection{collapsed,index};
      UpdateWindowState(state);
      return;
   }

   // Same type as before
   ValueSelection old = state->frame[frame].value();

   if(old.type == ValueSelection::EMBEDDED_LIST && old.val.custom == collapsed.custom){
      state->frame[frame].value().index = index;
   } else {
      state->frame[frame] = ValueSelection{collapsed,index};
   }

   if(index != WINDOW_STATE_INVALID_INDEX){
      UpdateWindowState(state);
   }
}

*/

void PrintWindowState(ValueWindowState* state){
   for(int i = 0; i < WINDOW_STATE_FRAME_SIZE; i++){
      printf("%d ",state->frame[i]);
   }
   printf("\n");
}

void DisplayValueWindow(const char* name,ValueWindowState* state,Arena* arena){
   if(!state->init){
      return;
   }

   BLOCK_REGION(arena);

   String strName = {};

   if(state->windowName){
      strName = PushString(arena,"%s##%s",name,state->windowName);
   } else {
      strName = PushString(arena,"%s",name);
   }

   PushNullByte(arena);

   if(ImGui::Begin(strName.data)){
      int frameSize = 0;
      for(frameSize = 0; frameSize < WINDOW_STATE_FRAME_SIZE; frameSize++){
         if(state->frame[frameSize] == WINDOW_STATE_INVALID_INDEX){
            break;
         }
      }

      if(state->startValue){
         frameSize += 1;
      }

      //printf("%d\n",frameSize);
      //PrintWindowState(state);

      // TODO: Need to collect the types of values before
      //       and if I detect a different type change, call ClearWindowState to the next rows

      if(frameSize && ImGui::BeginTable("split",frameSize + 1,ImGuiTableFlags_RowBg)){
         Value currentValue = *state->startValue;

         ImGui::TableNextRow();

         bool embeddedList = state->embbededListStart;
         for(int i = 0; i < WINDOW_STATE_FRAME_SIZE; i++){
            ImGui::TableNextColumn();

            Optional<ValueSelected> sel = {};
            if(embeddedList){
               Value collapsed = CollapsePtrIntoStruct(currentValue);
               sel = ShowTable(collapsed,arena,state->frame[i]);
            } else {
               sel = OutputDebugValues(currentValue,arena); // TODO: This function displays arrays and hashtables, but the rest of the code assumes that everything is a struct. Gives bug when trying to access a Array<T> or something like that, because the code below tries to access the struct instead of the element clicked
            }

            #if 1
            if(sel){
               state->frame[i] = sel.value().index;
            }

            // Next value
            if(embeddedList){
               Iterator iter = Iterate(currentValue);
               for(int index = 0; index < state->frame[i]; index++){
                  Advance(&iter);
               }

               Value subValue = GetValue(iter);
               currentValue = CollapsePtrIntoStruct(subValue);
               embeddedList = false;
            } else {
               Value subValue = AccessStruct(currentValue,state->frame[i]);
               currentValue = CollapsePtrIntoStruct(subValue);

               Type* type = currentValue.type;
               if(IsEmbeddedListKind(type)){
                  embeddedList = true;
               }
            }

            #endif
            if(state->frame[i] == WINDOW_STATE_INVALID_INDEX){
               break;
            }
         }
         ImGui::EndTable();
      }
   }
   ImGui::End();
}

void ShowConfigurations(Accelerator* accel,Arena* arena,Filter filter){
   BLOCK_REGION(arena);
   static int selected = 0;

   if(ImGui::BeginChild("Select",ImVec2(150.0f,0.0f),false)){
      if(ImGui::Selectable("Config",selected == 0)){
         selected = 0;
      }
      if(ImGui::Selectable("State",selected == 1)){
         selected = 1;
      }
      if(ImGui::Selectable("Delay",selected == 2)){
         selected = 2;
      }
      if(ImGui::Selectable("Mem",selected == 3)){
         selected = 3;
      }
      if(ImGui::Selectable("Outputs",selected == 4)){
         selected = 4;
      }
      if(ImGui::Selectable("ExtraData",selected == 5)){
         selected = 5;
      }
   }
   ImGui::EndChild();
   ImGui::SameLine();

   CalculatedOffsets offsets = {};

   filter.config = {};
   switch(selected){
   case 0:{
      offsets = ExtractConfig(accel,arena);
      filter.config.showConfig = true;
      filter.config.showStatic = true;
   }break;
   case 1:{
      offsets = ExtractState(accel,arena);
      filter.config.showState = true;
   }break;
   case 2:{
      offsets = ExtractDelay(accel,arena);
      filter.config.showDelay = true;
   }break;
   case 3:{
      offsets = ExtractMem(accel,arena);
      filter.config.showMem = true;
   }break;
   case 4:{
      offsets = ExtractOutputs(accel,arena);
      filter.config.showOutputs = true;
   }break;
   case 5:{
      offsets = ExtractExtraData(accel,arena);
      filter.config.showExtraData = true;
   }break;
   }

   if(ImGui::BeginTable("split",2,ImGuiTableFlags_RowBg)){
      AcceleratorIterator iter = {};

      int index = 0;
      for(InstanceNode* node = iter.Start(accel,arena,true); node; node = iter.Next(),index += 1){
         ComplexFUInstance* inst = node->inst;
         if(!PassFilter(accel,inst,filter)){
            continue;
         }
         BLOCK_REGION(arena);

         int offset = offsets.offsets[index];

         String fullName = iter.GetFullName(arena);

         ImGui::TableNextRow();
         ImGui::TableNextColumn(); ImGui::Text("[%d] %.*s",index,UNPACK_SS(fullName));
         ImGui::TableNextColumn();

         if(selected == 0){
            if(inst->isStatic){
               ImGui::Text("(Static:%d)",offset - offsets.max);
            } else if(inst->sharedEnable){
               BLOCK_REGION(arena);
               InstanceNode* parent = iter.ParentInstance();

               String parentName = STRING("TOP");
               if(parent){
                  parentName = iter.GetParentInstanceFullName(arena);
               }

               ImGui::Text("%d (Shared %.*s: %d)",offset,UNPACK_SS(parentName),inst->sharedIndex);
            } else {
               ImGui::Text("%d",offset);
            }
         } else {
            ImGui::Text("%d",offset);
         }
      }
      ImGui::EndTable();
   }
}

void OutputAcceleratorRunValues(InstanceNode* node,Arena* arena){
   BLOCK_REGION(arena);
   ComplexFUInstance* inst = node->inst;

   if(ImGui::CollapsingHeader("Config")){
      Array<int> arr = {};
      arr.data = inst->config;
      arr.size = inst->declaration->configs.size;

      ShowDualTable(MakeValue(&inst->declaration->configs,"Array<Wire>"),MakeValue(&arr,"Array<int>"),arena);
   }
   if(ImGui::CollapsingHeader("State")){
      Array<int> arr = {};
      arr.data = inst->state;
      arr.size = inst->declaration->states.size;

      ShowDualTable(MakeValue(&inst->declaration->states,"Array<Wire>"),MakeValue(&arr,"Array<int>"),arena);
   }
   if(ImGui::CollapsingHeader("Delay")){
      Array<int> arr = {};
      arr.data = inst->delay;
      arr.size = inst->declaration->nDelays;

      printf("%p %d\n",arr.data,arr.size);

      if(arr.size){
         ShowTable(MakeValue(&arr,"Array<int>"),arena);
      }
   }
   if(ImGui::CollapsingHeader("Output values")){
      Array<int> arr = {};
      arr.data = inst->outputs;
      arr.size = inst->declaration->outputLatencies.size;

      ShowTable(MakeValue(&arr,"Array<int>"),arena);
   }
   if(ImGui::CollapsingHeader("Stored Outputs")){
      Array<int> arr = {};
      arr.data = inst->storedOutputs;
      arr.size = inst->declaration->outputLatencies.size;

      ShowTable(MakeValue(&arr,"Array<int>"),arena);
   }
   if(ImGui::CollapsingHeader("Inputs")){
      int inputs = inst->declaration->inputDelays.size;
      Array<int> arr = PushArray<int>(arena,inputs);
      Array<String> names = PushArray<String>(arena,inputs);

      for(int i = 0; i < inputs; i++){
         arr[i] = GetInputValue(node,i);

         PortNode other = node->inputs[i];
         if(other.node){
            names[i] = PushString(arena,"%.*s:%d",UNPACK_SS(other.node->inst->name),other.port);
         } else {
            names[i] = STRING("Unconnected");
         }
      }

      ShowDualTable(MakeValue(&names,"Array<String>"),MakeValue(&arr,"Array<int>"),arena);
   }
   if(ImGui::CollapsingHeader("Outputs")){
      int outputs = Size(node->allOutputs);
      Array<String> portValue = PushArray<String>(arena,outputs);
      Array<String> names = PushArray<String>(arena,outputs);

      int i = 0;
      FOREACH_LIST_INDEXED(con,node->allOutputs,i){
         portValue[i] = PushString(arena,"Val: %d [Port: %d]",inst->outputs[con->port],con->port);
         names[i] = PushString(arena,"%.*s:%d",UNPACK_SS(con->instConnectedTo.node->inst->name),con->instConnectedTo.port);
      }

      ShowDualTable(MakeValue(&portValue,"Array<String>"),MakeValue(&names,"Array<String>"),arena);

   }
}

void OutputAcceleratorStaticValues(Accelerator* accel,Arena* arena){
   if(ImGui::CollapsingHeader("Static")){
      if(ImGui::BeginTable("split",2,ImGuiTableFlags_RowBg)){
         int index = 0;
         for(auto info : accel->staticInfo){
            BLOCK_REGION(arena);

            String repr1 = Repr(info->id,arena);
            String repr2 = Repr(info->data,arena);

            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::Text("[%d] %.*s",index++,UNPACK_SS(repr1));
            ImGui::TableNextColumn(); ImGui::Text("%.*s",UNPACK_SS(repr2));
         }
         ImGui::EndTable();
      }
   }
}

bool IsInspectable(Value val){
   Type* type = val.type;

   if(IsStruct(type)){
      return true;
   }
   if(IsIndexable(type)){
      return true;
   }

   return false;
}

struct DebugWindow{
   const char* label;
   Value val[99];
   int stackIndex;
   Optional<Accelerator*> accel;
   Optional<String> selectedInstance;
   bool value;
   bool inUse;
};

#define DEBUG_WINDOW_SIZE 10

// Debug gui state
static DebugWindow windows[DEBUG_WINDOW_SIZE] = {};
static Filter filter = {};

static Optional<String> selectedInstance = {};
static Optional<FUDeclaration*> selectedDeclaration = {};
static Optional<Accelerator*> selectedAccelerator = {};
static Optional<Value> selectedValue = {};
static Optional<int> cycle = {};
static bool last = false;

static ValueWindowState valueWindowState;

InstanceNode* GetInstanceFromName(Accelerator* accel,String string,Arena* arena){
   AcceleratorIterator iter = {};
   for(InstanceNode* node = iter.Start(accel,arena,false); node; node = iter.Next()){
      BLOCK_REGION(arena);
      String name = iter.GetFullName(arena);

      if(CompareString(name,string)){
         return node;
      }
   }
   Assert(false);
   return nullptr;
}

void DebugGUI(Arena* arena){
   static bool permanentlyDone = false;
   if(permanentlyDone){
      return;
   }

   SDL_Window* window = InitWindow();
   ImGuiIO& io = ImGui::GetIO();

   ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

   // Main loop
   bool done = false;
   while (!done){
      BLOCK_REGION(arena);

      SDL_Event event;
      while (SDL_PollEvent(&event)){
         ImGui_ImplSDL2_ProcessEvent(&event);
         if (event.type == SDL_QUIT){
            permanentlyDone = true;
            done = true;
         }
         if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window)){
            permanentlyDone = true;
            done = true;
         }
      }

      ImGui_ImplOpenGL3_NewFrame();
      ImGui_ImplSDL2_NewFrame();
      ImGui::NewFrame();

      #if 01
      ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
      ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
      ImGui::GetStyle().WindowRounding = 0.0f;
      ImGui::ShowDemoWindow(nullptr);
      #endif

      //ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_FirstUseEver);
      //ImGui::SetNextWindowSize(ImVec2(550, 380), ImGuiCond_FirstUseEver);

      if(ImGui::Begin("Control")){
         if(ImGui::Button("Close")){
            done = true;
         }
         ImGui::SameLine();
         if(ImGui::Button("Terminate debugging")){
            done = true;
            permanentlyDone = true;
         }

         if(cycle){
            if(last){
               ImGui::Text("Finished in %d cycles",cycle.value());
            } else if(cycle == -1){
               ImGui::Text("Accelerator Pre Start");
            } else {
               ImGui::Text("Current cycle: %d",cycle.value());
            }
         }
      }
      ImGui::End();

      #if 0
      for(int i = 0; i < DEBUG_WINDOW_SIZE; i++){
         DebugWindow* dw = &windows[i];
         if(!dw->inUse){
            continue;
         }

         if(ImGui::Begin(dw->label)){
            if(ImGui::BeginTabBar("TabBar")){
               if(dw->accel && ImGui::BeginTabItem("Accelerator")){
                  Accelerator* accel = dw->accel.value();
                  if(ImGui::BeginChild("Tree",ImVec2(200.0f,500.0f))){
                     AcceleratorIterator iter = {};
                     iter.Start(accel,arena,false);

                     Optional<String> res = AcceleratorTreeNodes(iter,filter);
                     if(res){
                        dw->selectedInstance = res;
                     }
                  }
                  ImGui::EndChild();

                  ImGui::SameLine();

                  if(dw->selectedInstance){
                     if(ImGui::BeginChild("Values")){
                        String name = dw->selectedInstance.value();
                        selectedDeclaration = GetInstanceFromName(accel,name,arena)->inst->declaration;
                        ComplexFUInstance* inst = (ComplexFUInstance*) &dw->selectedInstance.value();

                        Optional<ValueSelected> res = OutputDebugValues(MakeValue(inst,"ComplexFUInstance"),arena);

                        if(res){
                           Value val = CollapsePtrIntoStruct(res.value().val);
                           if(IsStruct(val.type)){
                              selectedValue = val;
                           }
                        }

                        OutputAcceleratorRunValues(inst,arena);
                        OutputAcceleratorStaticValues(accel,arena);
                     }
                     ImGui::EndChild();
                  }
                  ImGui::EndTabItem();
               }
               #if 1
               if(dw->value && ImGui::BeginTabItem("Values")){
                  ImGui::BeginGroup();
                  ImGui::Text("%d",dw->stackIndex);
                  if(ImGui::Button("<=")){
                     dw->stackIndex -= 1;
                     if(dw->stackIndex < 0){
                        dw->stackIndex = 0;
                     }
                  }
                  ImGui::SameLine();
                  ImGui::Text("%.*s",UNPACK_SS(dw->val[dw->stackIndex].type->name));
                  ImGui::EndGroup();

                  Optional<ValueSelected> res = OutputDebugValues(dw->val[dw->stackIndex],arena);

                  if(res){
                     Value trueVal = CollapsePtrIntoStruct(res.value().val);

                     if(IsInspectable(trueVal)){
                        dw->stackIndex += 1;
                        dw->val[dw->stackIndex] = res.value().val;
                     }
                  }
                  ImGui::EndTabItem();
               }
               #endif
               if(dw->accel && ImGui::BeginTabItem("Configurations")){
                  Accelerator* accel = dw->accel.value();
                  ShowConfigurations(accel,arena,filter);
                  ImGui::EndTabItem();
               }
               ImGui::EndTabBar();
            }
         }
         ImGui::End();
      }
      #endif

      ImGui::SetNextWindowPos(ImVec2(0.0f, 600.0f), ImGuiCond_FirstUseEver);
      ImGui::SetNextWindowSize(ImVec2(-1.0f, -1.0f), ImGuiCond_FirstUseEver);
      if(ImGui::Begin("Filter")){
         ImGui::BeginGroup();
         if(ImGui::Button("Check All")){
            memset(&filter.config,0x01,sizeof(filter.config));
         }
         ImGui::SameLine();
         if(ImGui::Button("Uncheck All")){
            memset(&filter.config,0x00,sizeof(filter.config));
         }
         ImGui::EndGroup();

         ImGui::Separator();

         ImGui::Checkbox("Config",&filter.config.showConfig);
         ImGui::Checkbox("Static",&filter.config.showStatic);
         ImGui::Checkbox("State",&filter.config.showState);
         ImGui::Checkbox("Delay",&filter.config.showDelay);
         ImGui::Checkbox("Mem",&filter.config.showMem);
         ImGui::Checkbox("Outputs",&filter.config.showOutputs);
         ImGui::Checkbox("ExtraData",&filter.config.showExtraData);

         ImGui::Separator();

         static int selected = 0;
         if(ImGui::RadioButton("Or",(selected == 0))){
            selected = 0;
         }
         ImGui::SameLine();
         if(ImGui::RadioButton("And",(selected == 1))){
            selected = 1;
         }

         filter.doAnd = selected;

         ImGui::Checkbox("Simple",&filter.types.showSimple);
         ImGui::SameLine();
         if(!filter.types.showSimple){
            filter.types.showNoOperation = false;
         }
         if(ImGui::RadioButton("No Operation",filter.types.showSimple && filter.types.showNoOperation)){
            filter.types.showSimple = true;
            filter.types.showNoOperation = !filter.types.showNoOperation;
         }

         ImGui::Checkbox("Composite",&filter.types.showComposite);
         ImGui::Checkbox("Special",&filter.types.showSpecial);
      }
      ImGui::End();

      DisplayValueWindow("Value Window",&valueWindowState,arena);

      if(selectedAccelerator){
         Accelerator* accel = selectedAccelerator.value();

         if(ImGui::Begin("Accelerator Units")){
            ImGui::BeginChild("Tree",ImVec2(200.0f,500.0f));   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
            AcceleratorIterator iter = {};
            iter.Start(accel,arena,false);

            Optional<String> res = AcceleratorTreeNodes(iter,filter);
            if(res){
               String name = res.value();
               printf("%.*s\n",UNPACK_SS(name));
               selectedInstance = res;
            }
            ImGui::EndChild();

            ImGui::SameLine();

            if(selectedInstance){
               String name = selectedInstance.value();
               InstanceNode* node = GetInstanceFromName(accel,name,arena);
               ComplexFUInstance* inst = node->inst;
               if(ImGui::BeginChild("Values")){
                  selectedDeclaration = inst->declaration;

                  Optional<ValueSelected> res = OutputDebugValues(MakeValue(inst,"ComplexFUInstance"),arena);

                  if(res){
                     Value val = CollapsePtrIntoStruct(res.value().val);
                     if(IsStruct(val.type)){
                        selectedValue = val;
                        AddToWindowState(&valueWindowState,val);
                     }
                  }

                  OutputAcceleratorRunValues(node,arena);
                  OutputAcceleratorStaticValues(accel,arena);
               }

               ImGui::EndChild();
            }
         }
         ImGui::End();

         if(ImGui::Begin("Accelerator")){
            Optional<ValueSelected> res = OutputDebugValues(MakeValue(accel,"Accelerator"),arena);

            if(res){
               Value val = CollapsePtrIntoStruct(res.value().val);
               if(IsStruct(val.type)){
                  selectedValue = val;
                  AddToWindowState(&valueWindowState,val);
               }
            }
         }
         ImGui::End();

         if(ImGui::Begin("Configurations")){
            ShowConfigurations(accel,arena,filter);
         }
         ImGui::End();

         //AcceleratorGraph(accel,arena,filter);
      }

      #if 0
      if(selectedDeclaration){
         if(ImGui::Begin("Declaration")){
            Optional<ValueSelected> res = OutputDebugValues(MakeValue(selectedDeclaration.value(),"FUDeclaration"),arena);

            if(res){
               Value val = CollapsePtrIntoStruct(res.value().val);
               if(IsStruct(val.type)){
                  AddToWindowState(&valueWindowState,val);
                  selectedValue = val;
               }
            }
         }
         ImGui::End();
      }
      #endif

      #if 0
      if(selectedValue){
         if(ImGui::Begin("Value viewer")){
            Value val = CollapsePtrIntoStruct(selectedValue.value());

            if(IsStruct(val.type)){
               Optional<Value> res = OutputDebugValues(selectedValue.value(),arena);
               selectedValue = OrElse(res,selectedValue);
            } else if(IsIndexable(val.type)){
               Optional<Value> res = ShowTable(val,arena);
               selectedValue = OrElse(res,selectedValue);
            }
         }
         ImGui::End();
      }
      #endif

      // Rendering
      ImGui::Render();
      glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
      glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
      glClear(GL_COLOR_BUFFER_BIT);
      ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
      SDL_GL_SwapWindow(window);
   }

   if(cycle){
      cycle = cycle.value() + 1;
   }
}

static void ClearState(){
   valueWindowState = {};
   selectedInstance = {};
   selectedDeclaration = {};
   selectedAccelerator = {};
   selectedValue = {};
   cycle = {};
   ClearWindowState(&valueWindowState,0);
   last = false;
}

void DebugAccelerator(Accelerator* accel,Arena* arena){
   selectedAccelerator = accel;
   DebugGUI(arena);

   ClearState();
}

void DebugAcceleratorPersist(Accelerator* accel,Arena* arena){
   if(!selectedAccelerator || (selectedAccelerator && selectedAccelerator.value() != accel)){
      ClearState();
   }

   selectedAccelerator = accel;
   DebugGUI(arena);
}

void DebugAcceleratorStart(Accelerator* accel,Arena* arena){
   if(!selectedAccelerator || (selectedAccelerator && selectedAccelerator.value() != accel)){
      ClearState();
   }

   cycle = -1;
   selectedAccelerator = accel;
   DebugGUI(arena);
}

void DebugAcceleratorEnd(Accelerator* accel,Arena* arena){
   last = true;
   DebugGUI(arena);
   ClearState();
}

void DebugWindowValue(const char* label,Value val){
   for(int i = 0; i < DEBUG_WINDOW_SIZE; i++){
      DebugWindow* dw = &windows[i];
      if(dw->inUse){
         continue;
      }

      dw->inUse = true;
      dw->label = label;
      dw->stackIndex = 0;
      dw->val[0] = val;
      dw->value = true;
      return;
   }

   printf("Too many debug windows open\n");
}

void DebugWindowAccelerator(const char* label,Accelerator* accel){
   for(int i = 0; i < DEBUG_WINDOW_SIZE; i++){
      DebugWindow* dw = &windows[i];
      if(dw->inUse){
         continue;
      }

      dw->inUse = true;
      dw->label = label;
      dw->stackIndex = 0;
      dw->accel = accel;
      return;
   }

   printf("Too many debug windows open\n");
}

void DebugValue(const char* windowName,Value val,Arena* arena){
   ClearState();

   valueWindowState.windowName = windowName;
   ClearWindowState(&valueWindowState,0);
   valueWindowState.startValue = val;

   DebugGUI(arena);
}

#endif
