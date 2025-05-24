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
    Notebook notebook;
    Toolbar toolbar;
    ToolButton item;

  public:
    MainWindow();
    virtual ~MainWindow()
    {
    }

    virtual bool handle(std::string_view) override;
    virtual void use(void *, RenderAtoms &, CanFormRectangle &, CanFormRectangle &) override;
};

int main(int argc, char **argv)
{
    srand(time(nullptr));
    Main kit(argc, argv);

    MainWindow mainWindow;
    Main::run(mainWindow);
    return 0;
}

MainWindow::MainWindow() : vBox(), notebook(), toolbar(), item("☰") // U+2630
{
    set_title("CanForm GTKMM Test");
    set_default_size(800, 600);
    maximize();
    add(vBox);

    vBox.pack_start(toolbar, PACK_SHRINK);

    toolbar.append(item);
    item.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::OnTool));

    vBox.pack_start(notebook, PACK_EXPAND_WIDGET);
    notebook.set_scrollable();
    notebook.popup_enable();

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
                makeForm(), "Non Modal Form",
                [this](Form &form, DialogResult result) { printForm(form, result, this); }, this);
            return false;
        });
        menu.add("Add Canvas", [this]() {
            getCanvasAtoms(randomString(5, 10), *this, &notebook);
            return true;
        });
    }

    menuList.show("Main Menu", this);
}

void MainWindow::use(void *, RenderAtoms &atoms, CanFormRectangle &viewRect, CanFormRectangle &viewBounds)
{
    atoms.clear();

    viewRect.x = 0;
    viewRect.y = 0;

    Gtk::Allocation allocation = get_allocation();
    viewRect.w = allocation.get_width();
    viewRect.h = allocation.get_height();

    viewBounds.x = -viewRect.w * 0.5;
    viewBounds.y = -viewRect.h * 0.5;
    viewBounds.w = viewRect.w * 2;
    viewBounds.h = viewRect.h * 2;

    const size_t n = rand() % 100 + 100;
    RandomRender random(viewRect.w, viewRect.h);
    for (size_t i = 0; i < n; ++i)
    {
        atoms.emplace_back(random());
    }

    {
        RenderAtom atom;
        atom.style.lineWidth = 1.0;
        atom.renderType.emplace<CanFormRectangle>(viewRect);
        atoms.emplace_back(std::move(atom));
    }
    {
        RenderAtom atom;
        atom.style.lineWidth = 1.0;
        atom.style.color = Color(1.f, 0.f, 1.f);
        atom.renderType.emplace<CanFormRectangle>(viewBounds);
        atoms.emplace_back(std::move(atom));
    }

    queue_draw();
}