#include <canform.hpp>
#include <filesystem>
#include <gtkmm.h>
#include <gtkmm/gtkmm.hpp>
#include <tests/test.hpp>

using namespace CanForm;
using namespace Gtk;

class MainWindow : public Window, public FileDialog::Handler
{
  private:
    void OnTool();
    void OnCreate();

    Form createForm() const;

    VBox box;
    FlowBox flowBox;
    SpinButton columns, bools, integers, floats, strings, selections, flags;
    Button button;
    Toolbar toolbar;
    ToolButton item;

    void addToSpinButton(Glib::ustring name, SpinButton &button, int min = 0, int max = 10)
    {
        button.set_range(min, max);
        button.set_increments(1, (max - min) / 10);
        Frame *frame = manage(new Frame(name));
        frame->add(button);
        flowBox.add(*frame);
    }

    template <typename T> void addToForm(Form &form, const SpinButton &button) const
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

    virtual bool handle(std::string_view) override;
};

int main(int argc, char **argv)
{
    srand(time(nullptr));
    Main kit(argc, argv);

    MainWindow mainWindow;
    Main::run(mainWindow);
    return 0;
}

MainWindow::MainWindow() : button("Create"), item("☰") // U+2630
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

    button.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::OnCreate));

    box.pack_start(flowBox, PACK_EXPAND_PADDING);

    show_all_children();
}

bool MainWindow::handle(std::string_view file)
{
    showMessageBox(MessageBoxType::Information, "Got File/Directory", file, this);
    return true;
}

void MainWindow::OnTool()
{
    MenuList menuList;
    {
        auto &menu = menuList.menus.emplace_back();
        menu.title = "File";
        menu.add("Open File", [this]() {
            FileDialog dialog;
            dialog.message = "Select directory(s)";
            auto s = std::filesystem::current_path().string();
            dialog.startDirectory = s;
            dialog.multiple = true;
            dialog.show(*this, this);
            return true;
        });
        menu.add("Open Directory", [this]() {
            FileDialog dialog;
            dialog.message = "Select file(s)";
            auto s = std::filesystem::current_path().string();
            dialog.startDirectory = s;
            dialog.multiple = true;
            dialog.directories = true;
            dialog.show(*this, this);
            return true;
        });
        menu.add("Save File", [this]() {
            FileDialog dialog;
            dialog.message = "Select file(s)";
            auto s = std::filesystem::current_path().string();
            dialog.startDirectory = s;
            dialog.saving = true;
            dialog.show(*this, this);
            return true;
        });
        menu.add("Exit", [this]() {
            hide();
            return true;
        });
    }
    {
        auto &menu = menuList.menus.emplace_back();
        menu.title = "Tests";
        menu.add("Modal Form", [this]() {
            Form form = makeForm();
            printForm(form, executeForm("Modal Form", form, 2, this), this);
            return false;
        });
        menu.add("Wait for 3 seconds", [this]() {
            showPopupUntil("Waiting...", std::chrono::seconds(3), 500, this);
            return false;
        });
        menu.add("Replace Menu", [this]() {
            MenuList menuList;
            auto &menu = menuList.menus.emplace_back();
            menu.title = "Secondary Menu";
            menu.add("Close", []() { return true; });
            return makeNewMenu<true>("New Menu", std::move(menuList));
        });
    }

    menuList.show("Main Menu", this);
}

void MainWindow::OnCreate()
{
    Form form = createForm();
    printForm(form, executeForm("Modal Form", form, columns.get_value(), this), this);
}

Form MainWindow::createForm() const
{
    Form form;
    addToForm<bool>(form, bools);
    addToForm<int64_t>(form, integers);
    addToForm<double>(form, floats);
    addToForm<String>(form, strings);
    addToForm<StringSelection>(form, selections);
    addToForm<StringMap>(form, flags);
    return form;
}