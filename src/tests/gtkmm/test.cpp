#include <canform.hpp>
#include <gtkmm/button.h>
#include <gtkmm/main.h>
#include <gtkmm/window.h>

using namespace CanForm;
using namespace Gtk;

class MainWindow : public Window
{
  public:
    MainWindow();
    virtual ~MainWindow()
    {
    }

  protected:
    void OnClick();

    Button button;
};

int main(int argc, char **argv)
{
    Main kit(argc, argv);

    MainWindow mainWindow;
    Main::run(mainWindow);
    return 0;
}

MainWindow::MainWindow() : button("Hello World")
{
    set_border_width(10);

    button.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::OnClick));

    add(button);
    button.show();
}

void MainWindow::OnClick()
{
    showMessageBox(MessageBoxType::Information, "Button Clicked", "The button was clicked", this);
}