#include <canform.hpp>
#include <filesystem>
#include <gtkmm.h>
#include <gtkmm/gtkmm.hpp>
#include <tests/test.hpp>

using namespace CanForm;
using namespace Gtk;

class MainWindow : public Window, public FileDialog::Handler, public RenderAtomsUser
{
  private:
    void OnTool();

    VBox vBox;
    HButtonBox buttonBox;
    Toolbar toolbar;
    ToolButton item;

  public:
    MainWindow();
    virtual ~MainWindow()
    {
    }

    virtual bool handle(std::string_view) override;
    virtual void use(RenderAtoms &, CanFormRectangle &) override;
};

int main(int argc, char **argv)
{
    Main kit(argc, argv);

    MainWindow mainWindow;
    Main::run(mainWindow);
    return 0;
}

MainWindow::MainWindow() : item("☰") // U+2630
{
    set_title("CanForm GTKMM Test");
    set_size_request(800, 600);
    add(vBox);

    vBox.pack_start(toolbar, PACK_SHRINK);
    buttonBox.set_border_width(5);
    buttonBox.set_layout(Gtk::BUTTONBOX_END);
    vBox.pack_end(buttonBox);

    toolbar.append(item);
    item.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::OnTool));

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
            printForm(form, executeForm("Modal Form", form, this), this);
            return false;
        });
        menu.add("Non Modal Form", [this]() {
            showAsyncForm(
                makeForm(), "Non Modal Form", [](Form &form, DialogResult result) { printForm(form, result); }, this);
            return false;
        });
        menu.add("Add Canvas", [this]() {
            // const Glib::ustring string = randomString(5, 10);
            // getCanvasAtoms(convert(string), *this, nullptr);
            return true;
        });
    }

    menuList.show("Main Menu", this);
}

void MainWindow::use(RenderAtoms &, CanFormRectangle &)
{
}