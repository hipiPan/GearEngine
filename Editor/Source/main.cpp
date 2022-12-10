#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <imgui.h>

#include <Window/BaseWindow.h>
#include <Application/BaseApplication.h>
#include <GearEngine.h>
#include <Renderer/Renderer.h>
#include <View/View.h>
#include <Entity/Scene.h>
#include <Entity/Entity.h>
#include <Entity/Components/CLight.h>
#include <Entity/Components/CCamera.h>
#include <Entity/Components/CTransform.h>
#include <Entity/Components/CMesh.h>
#include <Entity/Components/CSkybox.h>
#include <Entity/Components/CAnimation.h>
#include <Resource/Texture.h>
#include <Resource/Material.h>
#include <Resource/BuiltinResources.h>
#include <MaterialCompiler/MaterialCompiler.h>
#include <Animation/Skeleton.h>
#include <Animation/AnimationClip.h>
#include <Animation/AnimationInstance.h>
#include <Input/InputSystem.h>
#include <UI/Canvas.h>
#include <filesystem/path.h>

#include <map>

#include "CameraController.h"
#include "TestScene/TestScene.h"
#include "TestScene/AnimationTestScene.h"
#include "TestScene/ShadowTestScene.h"
#include "TestScene/MaterialTestScene.h"
#include "TestScene/TransparencyTestScene.h"
#include "TestScene/SkyAtmosphereTestScene.h"

class Window;
static std::map<GLFWwindow*, Window*> glfw_window_table;

class Window : public gear::BaseWindow {
public:
    Window(uint32_t w, uint32_t h) {
        glfw_window = glfwCreateWindow(w, h, "GearEditor", nullptr, nullptr);
        width = w;
        height = h;
        window_ptr = glfwGetWin32Window(glfw_window);
        glfwSetFramebufferSizeCallback(glfw_window, this->WindowSizeCallback);
        glfwSetCursorPosCallback(glfw_window, CursorPositionCallback);
        glfwSetMouseButtonCallback(glfw_window, MouseButtonCallback);
        glfwSetScrollCallback(glfw_window, MouseScrollCallback);
        glfw_window_table[glfw_window] = this;
    }

    ~Window() {
        glfwDestroyWindow(glfw_window);
    }

    bool ShouldClose() {
        return glfwWindowShouldClose(glfw_window);
    };

    static void WindowSizeCallback(GLFWwindow* window, int w, int h) {
        glfw_window_table[window]->InternalWindowSizeCallback(w, h);
    }

    static void CursorPositionCallback(GLFWwindow* window, double pos_x, double pos_y) {
        glfw_window_table[window]->InternalCursorPositionCallback(pos_x, pos_y);
    }

    static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
        glfw_window_table[window]->InternalMouseButtonCallback(button, action, mods);
    }

    static void MouseScrollCallback(GLFWwindow* window, double offset_x, double offset_y) {
        glfw_window_table[window]->InternalMouseScrollCallback(offset_x, offset_y);
    }

private:
    void InternalWindowSizeCallback(int w, int h) {
        width = w;
        height = h;
    }

    void InternalCursorPositionCallback(double pos_x, double pos_y) {
        gear::gEngine.GetInputSystem()->OnMousePosition(pos_x, pos_y);
    }

    void InternalMouseButtonCallback(int button, int action, int mods) {
        gear::gEngine.GetInputSystem()->OnMouseButton(button, action);
    }

    void InternalMouseScrollCallback(double offset_x, double offset_y) {
        gear::gEngine.GetInputSystem()->OnMouseScroll(offset_y);
    }

private:
    friend class Application;
    GLFWwindow* glfw_window = nullptr;
};

class Application : public gear::BaseApplication {
public:
    Application() {

    }

    ~Application() {

    }

    void Init() override {
        filesystem::path root_path = filesystem::path::getcwd();

        filesystem::path engine_resource_path = root_path / "../../Engine/BuiltinResources";
        gear::gEngine.GetBuiltinResources()->Initialize(engine_resource_path.str(filesystem::path::path_type::posix_path));

        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        // 初始化ImGui
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
        io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;
        io.BackendPlatformName = "glfw";
        io.KeyMap[ImGuiKey_Tab] = GLFW_KEY_TAB;
        io.KeyMap[ImGuiKey_LeftArrow] = GLFW_KEY_LEFT;
        io.KeyMap[ImGuiKey_RightArrow] = GLFW_KEY_RIGHT;
        io.KeyMap[ImGuiKey_UpArrow] = GLFW_KEY_UP;
        io.KeyMap[ImGuiKey_DownArrow] = GLFW_KEY_DOWN;
        io.KeyMap[ImGuiKey_PageUp] = GLFW_KEY_PAGE_UP;
        io.KeyMap[ImGuiKey_PageDown] = GLFW_KEY_PAGE_DOWN;
        io.KeyMap[ImGuiKey_Home] = GLFW_KEY_HOME;
        io.KeyMap[ImGuiKey_End] = GLFW_KEY_END;
        io.KeyMap[ImGuiKey_Insert] = GLFW_KEY_INSERT;
        io.KeyMap[ImGuiKey_Delete] = GLFW_KEY_DELETE;
        io.KeyMap[ImGuiKey_Backspace] = GLFW_KEY_BACKSPACE;
        io.KeyMap[ImGuiKey_Space] = GLFW_KEY_SPACE;
        io.KeyMap[ImGuiKey_Enter] = GLFW_KEY_ENTER;
        io.KeyMap[ImGuiKey_Escape] = GLFW_KEY_ESCAPE;
        io.KeyMap[ImGuiKey_KeyPadEnter] = GLFW_KEY_KP_ENTER;
        io.KeyMap[ImGuiKey_A] = GLFW_KEY_A;
        io.KeyMap[ImGuiKey_C] = GLFW_KEY_C;
        io.KeyMap[ImGuiKey_V] = GLFW_KEY_V;
        io.KeyMap[ImGuiKey_X] = GLFW_KEY_X;
        io.KeyMap[ImGuiKey_Y] = GLFW_KEY_Y;
        io.KeyMap[ImGuiKey_Z] = GLFW_KEY_Z;
        filesystem::path imgui_font_path = root_path / "../BuiltinResources/Fonts/Roboto-Medium.ttf";
        io.Fonts->AddFontFromFileTTF(imgui_font_path.str(filesystem::path::path_type::posix_path).c_str(), 16.0f);

        int tex_width, tex_height;
        unsigned char* pixels = nullptr;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &tex_width, &tex_height);
        auto font_tex_data = gear::TextureData::Builder()
            .SetWidth(tex_width)
            .SetHeight(tex_height)
            .SetFormat(blast::FORMAT_R8G8B8A8_UNORM)
            .SetData(pixels, tex_width * tex_height * 4)
            .Build();
        font_texture = font_tex_data->LoadTexture();

        filesystem::path imgui_material_path = root_path / "../BuiltinResources/Materials/ui.mat";
        imgui_ma = gear::gEngine.GetMaterialCompiler()->Compile(imgui_material_path.str(filesystem::path::path_type::posix_path), true);

        mouse_position_cb_handle = gear::gEngine.GetInputSystem()->GetOnMousePositionEvent().Bind(ImGuiMousePositionCB, 100);
        mouse_button_cb_handle = gear::gEngine.GetInputSystem()->GetOnMouseButtonEvent().Bind(ImGuiMouseButtonCB, 100);
        mouse_scroll_cb_handle = gear::gEngine.GetInputSystem()->GetOnMouseScrollEvent().Bind(ImGuiMouseScrollCB, 100);

        main_window = new Window(800, 600);
        scene_view = new gear::View();
        canvas = new gear::Canvas();

        test_scene = new ShadowTestScene();
        test_scene->Load();
    }

    void Exit() override {
        test_scene->Clear();
        SAFE_DELETE(test_scene);
        SAFE_DELETE(main_window);
        SAFE_DELETE(scene_view);
        SAFE_DELETE(canvas);

        ImGui::DestroyContext();
        glfwTerminate();
    };

    bool ShouldClose() override {
        if (main_window) {
            return main_window->ShouldClose();
        }
        return true;
    };

protected:
    void Tick(float dt) override {
        glfwPollEvents();

        // 构建ImGui
        ImGuiIO& io = ImGui::GetIO();
        io.DeltaTime = dt;
        io.DisplaySize = ImVec2((float)main_window->GetWidth(), (float)main_window->GetHeight());
        io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
        io.MousePos.x = gear::gEngine.GetInputSystem()->GetMousePosition().x;
        io.MousePos.y = gear::gEngine.GetInputSystem()->GetMousePosition().y;
        io.MouseWheel = gear::gEngine.GetInputSystem()->GetMouseScrollWheel();
        for (int i = 0; i < 3; i++) {
            io.MouseDown[i] = gear::gEngine.GetInputSystem()->GetMouseButtonDown(i) || gear::gEngine.GetInputSystem()->GetMouseButtonHeld(i);
        }

        ImGui::NewFrame();

        test_scene->DrawUI();

        ImGui::Render();

        canvas->Begin();
        ImDrawData* commands = ImGui::GetDrawData();
        uint32_t num_prims = 0;
        for (int cmdListIndex = 0; cmdListIndex < commands->CmdListsCount; cmdListIndex++) {
            const ImDrawList* cmds = commands->CmdLists[cmdListIndex];
            num_prims += cmds->CmdBuffer.size();
        }

        uint32_t previous_size = imgui_mis.size();
        if (num_prims > imgui_mis.size()) {
            imgui_mis.resize(num_prims);
            for (size_t i = previous_size; i < imgui_mis.size(); i++) {
                imgui_mis[i] = imgui_ma->CreateInstance();
                imgui_mis[i]->SetTexture("albedo_texture", font_texture);
                blast::GfxSamplerDesc sampler_desc;
                imgui_mis[i]->SetSampler("albedo_sampler", sampler_desc);
            }
        }

        uint32_t prim_index = 0;
        for (uint32_t cmd_list_index = 0; cmd_list_index < commands->CmdListsCount; cmd_list_index++) {
            const ImDrawList* cmds = commands->CmdLists[cmd_list_index];
            uint32_t index_offset = 0;

            gear::Canvas::Batch canvas_batch;
            canvas_batch.vertex_data = (uint8_t*)(cmds->VtxBuffer.Data);
            canvas_batch.vertex_count = cmds->VtxBuffer.Size;
            canvas_batch.vertex_layout = gear::VLT_UI;
            canvas_batch.index_data = (uint8_t*)(cmds->IdxBuffer.Data);
            canvas_batch.index_count = cmds->IdxBuffer.Size;
            canvas_batch.index_type = blast::INDEX_TYPE_UINT16;

            for (const auto& pcmd : cmds->CmdBuffer) {
                if (pcmd.UserCallback) {
                    pcmd.UserCallback(cmds, &pcmd);
                } else {
                    gear::Canvas::Element canvas_element;
                    std::shared_ptr<gear::MaterialInstance> imgui_mi = imgui_mis[prim_index];
                    imgui_mi->SetScissor( pcmd.ClipRect.x, pcmd.ClipRect.y, pcmd.ClipRect.z, pcmd.ClipRect.w);
                    canvas_element.mi = imgui_mi.get();
                    canvas_element.count = pcmd.ElemCount;
                    canvas_element.offset = index_offset;
                    canvas_batch.elements.push_back(canvas_element);
                    prim_index++;
                }
                index_offset += pcmd.ElemCount;
            }
            canvas->AddBatch(canvas_batch);
        }
        canvas->End();

        // 绘制场景
        scene_view->SetSize(main_window->GetWidth(), main_window->GetHeight());
        scene_view->SetViewport(0, 0, main_window->GetWidth(), main_window->GetHeight());
        gear::gEngine.GetRenderer()->RenderScene(test_scene->GetScene().get(), scene_view);

        // 绘制到window上
        gear::View* views[] = { scene_view };
        gear::Canvas* canvases[] = { canvas };
        gear::gEngine.GetRenderer()->RenderWindow(main_window, 1, views, 1, canvases);
    };

    static void ImGuiMousePositionCB(float x, float y) {
        // 当鼠标处于imgui控件内，则截断输入对应的输入事件
        if (ImGui::IsWindowHovered(1 << 2)) {
            gear::gEngine.GetInputSystem()->GetOnMousePositionEvent().Block();
        }
    }

    static void ImGuiMouseButtonCB(int button, int action) {
        // 当鼠标处于imgui控件内，则截断输入对应的输入事件
        if (ImGui::IsWindowHovered(1 << 2)) {
            gear::gEngine.GetInputSystem()->GetOnMouseButtonEvent().Block();
        }
    }

    static void ImGuiMouseScrollCB(float offset) {
        // 当鼠标处于imgui控件内，则截断输入对应的输入事件
        if (ImGui::IsWindowHovered(1 << 2)) {
            gear::gEngine.GetInputSystem()->GetOnMouseScrollEvent().Block();
        }
    }

private:
    Window* main_window = nullptr;
    gear::Canvas* canvas = nullptr;
    gear::View* scene_view = nullptr;
    std::shared_ptr<gear::Material> imgui_ma;
    std::vector<std::shared_ptr<gear::MaterialInstance>> imgui_mis;
    std::shared_ptr<blast::GfxTexture> font_texture;
    TestScene* test_scene = nullptr;
    EventHandle mouse_position_cb_handle;
    EventHandle mouse_button_cb_handle;
    EventHandle mouse_scroll_cb_handle;
};

int main() {
    double time = 0.0;
    Application app;
    app.Init();
    while (!app.ShouldClose()) {
        double current_time = glfwGetTime();
        float dt = time > 0.0 ? (float)(current_time - time) : (float)(1.0f / 60.0f);
        time = current_time;
        app.Run(dt);
    }
    app.Exit();
    return 0;
}