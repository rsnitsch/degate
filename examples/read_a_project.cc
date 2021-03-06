#include "ProjectImporter.h"
#include "Project.h"

#include <string>

using namespace degate;

int main(void) {

  // Create a project importer object.

  ProjectImporter importer;

  /*
    Specify the filename to open. You can give a directory as well.
    The project importer will look for the default project filename
    within the directory.
  */

  string filename("libtest/testfiles/project.xml");

  /*
    Import the project file, the cell library (gate_library.xml) and
    the logic model (lmodel.xml).
  */
  Project_shptr prj(importer.import_all(filename));

  /*
   Print project information to stdout.
  */

  prj->print_all(cout);

  return 1;
}
