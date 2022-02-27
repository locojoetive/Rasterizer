#include "Window.h"
#include <vector>


/*
* override ImGui Combo and ListBox to support std::vector without copying the values
* https://eliasdaler.github.io/using-imgui-with-sfml-pt2/#combobox-listbox
*/
namespace ImGui
{
	static auto vector_getter = [](void* vec, int idx, const char** out_text)
	{
		auto& vector = *static_cast<std::vector<std::string>*>(vec);
		if (idx < 0 || idx >= static_cast<int>(vector.size())) { return false; }
		*out_text = vector.at(idx).c_str();
		return true;
	};

	bool Combo(const char* label, int* currIndex, std::vector<std::string>& values)
	{
		if (values.empty()) { return false; }
		return Combo(label, currIndex, vector_getter, static_cast<void*>(&values), values.size());
	}

	bool ListBox(const char* label, int* currIndex, std::vector<std::string>& values)
	{
		if (values.empty()) { return false; }
		return ListBox(label, currIndex, vector_getter, static_cast<void*>(&values), values.size());
	}

}

Window::Window(int _width, int _height, std::string _name)
	: width(_width), height(_height), name(_name)
{
	InitGLFWwindow();
	
	sceneCamera = new Camera(
		width,
		height,
		glm::vec3(0, 0, 2)
	);

	scene = new Scene(*sceneCamera);

	InitImGUI();
}

Window::~Window() { closeWindow(); }

bool Window::shouldClose() { return glfwWindowShouldClose(windowObject); }

void Window::Update()
{
	// Calculate frame time and FPS
	currentTime = glfwGetTime();
	timeDifference = currentTime - previousTime;
	counter++;
	if (timeDifference >= 1.0 / 60.0)
	{
		std::string FPS = std::to_string((1.0 / timeDifference) * counter);
		std::string ms = std::to_string((timeDifference / counter) * 1000);
		std::string newTitle = "3D-Rasterizer - (" + FPS + "FPS / " + ms + "ms";
		glfwSetWindowTitle(windowObject, newTitle.c_str());
		previousTime = currentTime;
		// handles camera inputs
		HandleInputs();
	}
	glClearColor(0.07f, 0.13f, 0.17f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	scene->UpdateScene();

	RenderImGUI();
	HandleImGUIInputs();

	// Swap front with back buffer
	glfwSwapBuffers(windowObject);
	// Take care of all GLFW events
	glfwPollEvents();
}

void Window::closeWindow()
{
	// Deletes all ImGUI instances
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	glfwDestroyWindow(windowObject);
	glfwTerminate();
}

void Window::InitGLFWwindow()
{
	// Initialize GLFW
	glfwInit();
	/* WINDOW */
	// Tell GLFW what version of OpenGL we are using
	// We are using OpenGL 3.3
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	// Tell GLFW we are using the CORE profile
	// That means we only have the modern functions
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	// Create a GLFWwindow object
	windowObject = glfwCreateWindow(width, height, name.c_str(), NULL, NULL);
	if (windowObject == NULL)
	{
		closeWindow();
		glfwTerminate();
		throw std::runtime_error("Failed to create GLFW window");
	}

	// Introduce the window
	glfwMakeContextCurrent(windowObject);
	// Load GLAD so it configures OpenGL
	gladLoadGL();
	// Specify the view port
	glViewport(0, 0, width, height);

	glClearColor(0.07f, 0.13f, 0.17f, 1.0f);
}

void Window::InitImGUI()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	(void)io;
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(windowObject, true);
	ImGui_ImplOpenGL3_Init("#version 330");
}

void Window::RenderImGUI()
{
	// Tell OpenGL a new frame is about to begin
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	// ImGUI window creation
	ImGui::Begin("Inspect Model");

	std::vector<std::string> modelNames = scene->getModelNames();
	if (ImGui::Combo("Selected Model " + selectedItemIndex, &selectedItemIndex, modelNames))
	{
		scene->selectModel(selectedItemIndex);
		position = scene->selectedModel->getPosition();
		rotation = scene->selectedModel->getRotation();
		scale = scene->selectedModel->getScale();
	}

	ImGui::InputText("Path", pathInputText, IM_ARRAYSIZE(pathInputText));
	if (ImGui::Button("Load Model"))
	{
		scene->LoadModel(pathInputText);
	}

	if (modelNames.size() > 0)
	{
		if (ImGui::CollapsingHeader("Position"))
		{
			ImGui::SliderFloat("PosX", &position.x, -100.f, 100.f);
			ImGui::SliderFloat("PosY", &position.y, -100.f, 100.f);
			ImGui::SliderFloat("PosZ", &position.z, -100.f, 100.f);
		}

		if (ImGui::CollapsingHeader("Rotation"))
		{
			ImGui::SliderFloat("RotX", &rotation.x, 0.f, 360.f);
			ImGui::SliderFloat("RotY", &rotation.y, 0.f, 360.f);
			ImGui::SliderFloat("RotZ", &rotation.z, 0.f, 360.f);
		}

		if (ImGui::CollapsingHeader("Scale"))
		{
			ImGui::SliderFloat("ScaleX", &scale.x, 0.001f, 1.f);
			ImGui::SliderFloat("ScaleY", &scale.y, 0.001f, 1.f);
			ImGui::SliderFloat("ScaleZ", &scale.z, 0.001f, 1.f);
		}

		ImGui::ColorEdit4("Light Color", lightColor);
	}

	ImGui::SliderFloat("Camera Speed", &cameraSpeed, 0.f, 4.f);
	// Fancy color editor that appears in the window
	// Ends the window
	ImGui::End();

	// Renders the ImGUI elements
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	
	isKeyboardLockedByImGUI = ImGui::GetIO().WantCaptureKeyboard;
	isMouseLockedByImGUI = ImGui::GetIO().WantCaptureMouse;
}

void Window::HandleImGUIInputs()
{
	if (scene->isModelSelected())
	{
		scene->selectedModel->setPosition(position);
		scene->selectedModel->setRotation(rotation);
		scene->selectedModel->setScale(scale);
	}
	sceneCamera->speed = cameraSpeed;
	scene->light.setColor(glm::vec4(lightColor[0], lightColor[1], lightColor[2], lightColor[3]));
}

void Window::HandleInputs()
{
	// stores coordinates of the cursor
	double mouseX;
	double mouseY;

	// fetches the coordinates of the cursor
	glfwGetCursorPos(windowObject, &mouseX, &mouseY);

	if (!isKeyboardLockedByImGUI)
	{
		sceneCamera->KeyboardInputs(windowObject);
	}
	if (!isMouseLockedByImGUI)
	{
		sceneCamera->MouseInputs(windowObject, mouseX, mouseY);
	}
}