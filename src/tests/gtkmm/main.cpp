#include <canform.hpp>
#include <filesystem>
#include <fstream>
#include <gtkmm.h>
#include <gtkmm/gtkmm.hpp>
#include <gtkmm/window.hpp>
#include <tests/test.hpp>

using namespace CanForm;
using namespace Gtk;

class MainWindow : public Window
{
  private:
    void OnTool();
    void OnCreate();

    StructForm createForm() const;

    VBox box;
    FlowBox flowBox;
    SpinButton columns, bools, integers, floats, strings, selections, flags;
    Gtk::Image info, error, warning;
    Button button;
    Toolbar toolbar;
    ToolButton item;
    Gtk::Window *blockWindow;

    void addToSpinButton(Glib::ustring name, SpinButton &button, int min = 0, int max = 10)
    {
        button.set_range(min, max);
        button.set_increments(1, (max - min) / 10);
        Frame *frame = manage(new Frame(name));
        frame->add(button);
        flowBox.add(*frame);
    }

    template <typename T> void addToForm(StructForm &form, const SpinButton &button) const
    {
        for (size_t i = 0; i < button.get_value(); ++i)
        {
            addForm(form, randomString(3, 8), T());
        }
    }

  public:
    MainWindow();
    virtual ~MainWindow()
    {
    }
};

class Handler : public FileDialog::Handler
{
  private:
    Gtk::Window *window;
    constexpr Handler(Gtk::Window *w) noexcept : window(w)
    {
    }

  public:
    virtual ~Handler()
    {
    }

    virtual bool handle(std::string_view file) override
    {
        showMessageBox(MessageBoxType::Information, "Got File/Directory", file, window);
        return true;
    }

    static std::shared_ptr<Handler> create(Gtk::Window *w)
    {
        return std::shared_ptr<Handler>(new Handler(w));
    }
};

int main(int argc, char **argv)
{
    srand(time(nullptr));
    Main kit(argc, argv);

    MainWindow mainWindow;
    Main::run(mainWindow);
    return 0;
}

MainWindow::MainWindow() : button("Create"), item("☰") /*U+2630*/, blockWindow(nullptr)
{
    set_title("CanForm GTKMM Test");
    set_default_size(800, 600);
    maximize();
    Window::add(box);

    box.pack_start(toolbar, PACK_SHRINK);

    toolbar.append(item);
    item.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::OnTool));

    addToSpinButton("Booleans", bools);
    addToSpinButton("Integers", integers);
    addToSpinButton("Floats", floats);
    addToSpinButton("Strings", strings);
    addToSpinButton("Selections", selections);
    addToSpinButton("Flags", flags);
    addToSpinButton("Columns", columns, 1, 10);

    flowBox.add(button);

    try
    {
#if NDEBUG
#else
        auto icons = Gtk::IconTheme::get_default();
        std::ofstream file("icons.txt");
        for (auto icon : icons->list_icons())
        {
            file << icon.c_str() << std::endl;
        }
#endif
        info.set(icons->load_icon(getIconName(MessageBoxType::Information), 32));
        flowBox.add(info);
        warning.set(icons->load_icon(getIconName(MessageBoxType::Warning), 32));
        flowBox.add(error);
        error.set(icons->load_icon(getIconName(MessageBoxType::Error), 32));
        flowBox.add(warning);
    }
    catch (const std::exception &)
    {
    }

    button.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::OnCreate));

    box.pack_start(flowBox, PACK_EXPAND_PADDING);

    show_all_children();
}

void MainWindow::OnTool()
{
    MenuList menuList;
    {
        auto &menu = menuList.menus.emplace_back();
        menu.title = "File";
        menu.add("Open File", [this]() {
            FileDialog dialog;
            dialog.message = "Select file(s)";
            auto s = std::filesystem::current_path().string();
            dialog.startDirectory = s;
            dialog.multiple = true;
            dialog.show(Handler::create(this), this);
            return MenuState::Close;
        });
        menu.add("Open Directory", [this]() {
            FileDialog dialog;
            dialog.message = "Select directory(s)";
            auto s = std::filesystem::current_path().string();
            dialog.startDirectory = s;
            dialog.multiple = true;
            dialog.directories = true;
            dialog.show(Handler::create(this), this);
            return MenuState::Close;
        });
        menu.add("Save File", [this]() {
            FileDialog dialog;
            dialog.message = "Select file to save";
            auto s = std::filesystem::current_path().string();
            dialog.startDirectory = s;
            dialog.saving = true;
            dialog.show(Handler::create(this), this);
            return MenuState::Close;
        });
        menu.add("Exit", [this]() {
            hide();
            return MenuState::Close;
        });
    }
    {
        auto &menu = menuList.menus.emplace_back();
        menu.title = "Tests";
        menu.add("Show Example Form", [this]() {
            auto formExecute = executeForm([this](const Form &form) { printForm(form, this); }, makeForm());
            FormExecute::execute("Modal Form", std::move(formExecute), this);
            return MenuState::KeepOpen;
        });
        menu.add("Wait for 3 seconds", [this]() {
            showPopupUntil("Waiting...", std::chrono::seconds(3), 500, this);
            return MenuState::KeepOpen;
        });
        menu.add("Replace Menu", [this]() {
            MenuList menuList;
            auto &menu = menuList.menus.emplace_back();
            menu.title = "Secondary Menu";
            menu.add("Close", []() { return MenuState::Close; });
            return makeNewMenu("New Menu", std::move(menuList));
        });
        menu.add("Long Menu", [this]() {
            MenuList menuList;
            auto &menu = menuList.menus.emplace_back();
            menu.title = "Long Menu";
            for (size_t i = 0; i < 100; ++i)
            {
                char buffer[1024];
                std::snprintf(buffer, sizeof(buffer), "#%zu", i + 1);
                menu.add(buffer, []() { return MenuState::Close; });
            }
            return makeNewMenu("Long Menu", std::move(menuList));
        });
    }
    {
        auto &menu = menuList.menus.emplace_back();
        menu.title = "Modals";
        menu.add("Information", [this]() {
            showMessageBox(MessageBoxType::Information, "Error", "This is information", this);
            return MenuState::KeepOpen;
        });
        menu.add("Warning", [this]() {
            showMessageBox(MessageBoxType::Warning, "Warning", "This is a warning", this);
            return MenuState::KeepOpen;
        });
        menu.add("Error", [this]() {
            showMessageBox(MessageBoxType::Error, "Error", "This is an error", this);
            return MenuState::KeepOpen;
        });
        menu.add("Question", [this]() {
            askQuestion("Question", "Yes or No?", SimpleResponse(this), this);
            return MenuState::KeepOpen;
        });
        menu.add("Blocking", [this]() {
            puts("Blocking");
            blockWindow = showMessageBox(MessageBoxType::Warning, "Warning", "This message box will block execution",
                                         this, [this]() { blockWindow->hide(); });
            Main::run(*blockWindow);
            puts("Done Blocking");
            return MenuState::KeepOpen;
        });
    }

    MenuList::show("Main Menu", std::move(menuList), this);
}

void MainWindow::OnCreate()
{
    auto map = createForm();
    map.columns = columns.get_value();
    auto formExecute = executeForm([this](const Form &form) { printForm(form, this); }, std::in_place, std::move(map));
    FormExecute::execute("Modal Form", std::move(formExecute), this);
}

StructForm MainWindow::createForm() const
{
    StructForm structForm;
    addToForm<bool>(structForm, bools);
    addToForm<int64_t>(structForm, integers);
    addToForm<double>(structForm, floats);
    addToForm<String>(structForm, strings);
    addToForm<StringSelection>(structForm, selections);
    addToForm<StringMap>(structForm, flags);
    return structForm;
}