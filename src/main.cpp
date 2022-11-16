#define _CRT_SECURE_NO_WARNINGS 1
#include <glfw3.h>


#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include "ImNodesEz.h"


#include <vector>
#include <unordered_map>
#include <string>
#include <algorithm>

#define WIN_WIDTH 1920
#define WIN_HEIGHT 1080



struct Node
{
	int val;
	Node* l;
	Node* r;
};

struct InputTextCallback_UserData
{
	std::string* Str;
	ImGuiInputTextCallback  ChainCallback;
	void* ChainCallbackUserData;
};

std::string input{100};

struct Funcs
{


	static int MyResizeCallback(ImGuiInputTextCallbackData* data)
	{

		InputTextCallback_UserData* user_data = (InputTextCallback_UserData*)data->UserData;
		if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
		{
			// Resize string callback
			// If for some reason we refuse the new length (BufTextLen) and/or capacity (BufSize) we need to set them back to what we want.
			std::string* str = user_data->Str;
			IM_ASSERT(data->Buf == str->c_str());
			str->resize(data->BufTextLen);
			data->Buf = (char*)str->c_str();
		}
		else if (user_data->ChainCallback)
		{
			// Forward to user callback, if any
			data->UserData = user_data->ChainCallbackUserData;
			return user_data->ChainCallback(data);
		}
		return 0;
	}

	static bool MyInputTextMultiline(const char* label, std::string* str, const ImVec2& size, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data)
	{
		IM_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
		flags |= ImGuiInputTextFlags_CallbackResize;

		InputTextCallback_UserData cb_user_data;
		cb_user_data.Str = str;
		cb_user_data.ChainCallback = callback;
		cb_user_data.ChainCallbackUserData = user_data;
		return ImGui::InputTextMultiline(label, (char*)str->c_str(), str->capacity() + 1, size, flags, Funcs::MyResizeCallback, &cb_user_data);
	}
};


struct NodeGraph
{
	uint64_t ptr;
	ImNodes::Ez::SlotInfo parent[1];
	ImNodes::Ez::SlotInfo children[2];
	ImVec2 pos;
	bool selected;
};

struct BSTNode
{
	uint64_t ptr;
	int data;
	uint64_t left;
	uint64_t right;
	int level;
};

std::vector<BSTNode> nodes{};
std::vector<NodeGraph> graphNodes{};
std::unordered_map<uint64_t, size_t> nodeMap{};
std::unordered_map<uint64_t, size_t> graphNodeMap{};
std::unordered_map<uint64_t, NodeGraph*> relationShips{};

#include <sstream>
void UpdateThingy()
{
	nodes.clear();
	nodeMap.clear();
	graphNodes.clear();

	std::stringstream ss(input.c_str());
	std::string token;

	
	while (std::getline(ss, token))
	{
		std::replace(token.begin(), token.end(), ',', ' ');
		uint32_t ptr, left, right;
		int data, level;
		int argsMatched = sscanf(token.c_str(), "%lx%d%lx%lx%d", &ptr, &data, &left, &right, &level);
		nodes.emplace_back(BSTNode{ ptr, data, left, right, level });
		nodeMap.emplace(std::pair<uint64_t, size_t>(ptr, nodes.size()-1));


	}
	std::vector<float> yLevels((int)std::sqrt(nodes.size()), 0.0f);
	for (BSTNode& node : nodes)
	{
		float x = 0 + (200.0f * node.level);
		float y = yLevels[node.level] + 50.0f;
		yLevels[node.level] += 50;
		graphNodes.emplace_back(NodeGraph{ node.ptr, { "Parent", 1 }, {{ "Left", 1 }, { "Right", 1 }}, ImVec2{x, y}, false });
		graphNodeMap.emplace(std::pair<uint64_t, size_t>(node.ptr, graphNodes.size()-1));
	}
	for (BSTNode& node : nodes)
	{
		relationShips.emplace(std::pair<uint64_t, NodeGraph*>(node.ptr, &graphNodes[graphNodeMap[node.ptr]]));
	}
	int i = 0;

	return;
}

struct MousePos
{
	double x, y;
};


int main()
{
	if (!glfwInit()) return -1;

	GLFWwindow* window = glfwCreateWindow(WIN_WIDTH, WIN_HEIGHT, "Tree Visualiser", NULL, NULL);
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);


	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImNodes::Ez::Context* ImNodeCtx = ImNodes::Ez::CreateContext();
	IM_UNUSED(ImNodeCtx);

	ImNodes::CanvasState canvasState;


	ImGuiIO& io = ImGui::GetIO();

	ImGui::StyleColorsDark();

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 130");
	bool windowOpen = true;
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;
	const ImGuiViewport* viewport = ImGui::GetMainViewport();


	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		static bool use_work_area = true;
		ImGui::SetNextWindowPos(use_work_area ? viewport->WorkPos : viewport->Pos);
		ImGui::SetNextWindowSize(use_work_area ? viewport->WorkSize : viewport->Size);

		if (ImGui::Begin("Tree Visualiser", &windowOpen, flags))
		{
			ImGuiWindowFlags window_flags = ImGuiWindowFlags_None;

			ImGui::BeginChild("Text Area", ImVec2(ImGui::GetFontSize()*50, 0), true, window_flags);
			Funcs::MyInputTextMultiline("Text Area", &input, ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 100), 0, nullptr, (void*)&input);
			ImGui::Text("Nodes: %d\n", std::count(input.begin(), input.end(), '\n'));
			
			if (ImGui::Button("Visualise", ImVec2(ImGui::GetContentRegionAvail().x, 0))) UpdateThingy();
			ImGui::EndChild();

			ImGui::SameLine();
			ImGui::BeginChild("Node Area", ImVec2(0, 0), true, window_flags);
			

			ImNodes::Ez::BeginCanvas();
			ImNodes::BeginCanvas(&canvasState);
			
			for (NodeGraph& node : graphNodes)
			{
				BSTNode& bst = nodes[nodeMap[node.ptr]];
				if (ImNodes::Ez::BeginNode(&node, std::to_string(bst.data).c_str(), &node.pos, &node.selected))
				{
					ImNodes::Ez::InputSlots(node.parent, 1);
					ImNodes::Ez::OutputSlots(node.children, 2);
					ImNodes::Ez::EndNode();
				}
			}

			for (NodeGraph& node : graphNodes)
			{
				
				BSTNode& bst = nodes[nodeMap[node.ptr]];
				NodeGraph* leftNode = nullptr;
				if(bst.left != 0)	leftNode = relationShips[bst.left];
				NodeGraph* rightNode = nullptr;
				if(bst.right != 0)	rightNode = relationShips[bst.right];
				if(rightNode) ImNodes::Ez::Connection(rightNode, "Parent", &node, "Right");
				if(leftNode) ImNodes::Ez::Connection(leftNode, "Parent", &node, "Left");
			}
			ImNodes::EndCanvas();
			ImNodes::Ez::EndCanvas();

			ImGui::EndChild();
		}
		

		if (ImGui::IsMouseDragging(1))
		{
			const ImVec2& delta = ImGui::GetMouseDragDelta(1);
			canvasState.Offset.x += delta.x;
			canvasState.Offset.y += delta.y;
			
		}


		ImGui::End();
		ImGui::Render();
		glViewport(0, 0, WIN_WIDTH, WIN_HEIGHT);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());



		glfwSwapBuffers(window);
	}
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}