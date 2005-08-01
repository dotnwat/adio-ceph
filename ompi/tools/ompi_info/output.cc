//
// Copyright (c) 2004-2005 The Trustees of Indiana University.
//                         All rights reserved.
// Copyright (c) 2004-2005 The Trustees of the University of Tennessee.
//                         All rights reserved.
// Copyright (c) 2004-2005 High Performance Computing Center Stuttgart, 
//                         University of Stuttgart.  All rights reserved.
// Copyright (c) 2004-2005 The Regents of the University of California.
//                         All rights reserved.
// $COPYRIGHT$
// 
// Additional copyrights may follow
// 
// $HEADER$
//

#include "ompi_config.h"

#include <iostream>
#include <string>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "tools/ompi_info/ompi_info.h"

using namespace std;
using namespace ompi_info;

#define OMPI_max(a,b) (((a) > (b)) ? (a) : (b)) 


//
// Private variables
//

static int centerpoint = 24;
static int screen_width = 78;


// 
// Prints the passed strings in a pretty or parsable format.
//
void ompi_info::out(const string& pretty_message, const string &plain_message,
                    const string& value)
{
#ifdef HAVE_ISATTY
  // If we have isatty(), if this is not a tty, then disable
  // wrapping for grep-friendly behavior

  if (0 == isatty(0)) {
      screen_width = INT_MAX;
  }
#endif

  if (pretty) {
    string::size_type pos, max_value_width;
    string spaces;
    string v = value;
    string filler;

    int num_spaces = centerpoint - pretty_message.length();
    if (num_spaces > 0) {
      spaces = string(num_spaces, ' ');
    }

    max_value_width = screen_width - spaces.length() -
      pretty_message.length() - 2;
    if (!pretty_message.empty()) {
        filler = spaces + pretty_message + ": ";
    } else {
        filler = spaces + "  ";
    }

    while (true) {
      if (v.length() < max_value_width) {
        cout << filler << v << endl;
        break;
      } else {
        string spaces(centerpoint + 2, ' ');

        // Work backwards to find the first space before
        // max_value_width

        pos = v.rfind(' ', max_value_width);
        if (string::npos == pos) {

          // No space found < max_value_width.  Look for the first
          // space after max_value_width.

          pos = v.find(' ', max_value_width);

        if (string::npos == pos) {

            // There's just no spaces.  So just print it and be done.

            cout << filler << v << endl;
            break;
          } else {
            cout << filler << v.substr(0, pos) << endl;
            v = v.substr(pos + 1);
          }
        } else {
          cout << filler << v.substr(0, pos) << endl;
          v = v.substr(pos + 1);
        }

        // Reset for the next iteration

        filler = spaces;
      }
    }
  } else {
      if (!plain_message.empty()) {
          cout << plain_message << ":" << value << endl;
      } else {
          cout << value << endl;
      }
  }
}


// 
// Prints the passed integer in a pretty or parsable format.
//
void ompi_info::out(const string& pretty_message, const string &plain_message, 
                    int value)
{
  if (ompi_info::pretty) {
    string spaces(OMPI_max(centerpoint - pretty_message.length(), 0), ' ');
    if (!pretty_message.empty()) {
        cout << spaces << pretty_message << ": " << value << endl;
    } else {
        cout << spaces << "  " << value << endl;
    }
  } else {
      if (!plain_message.empty()) {
          cout << plain_message << ":" << value << endl;
      } else {
          cout << value << endl;
      }
  }
}
