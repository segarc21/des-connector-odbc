// Copyright (c) 2012, 2024, Oracle and/or its affiliates.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, version 2.0, as
// published by the Free Software Foundation.
//
// This program is designed to work with certain software (including
// but not limited to OpenSSL) that is licensed under separate terms, as
// designated in a particular file or component or in included license
// documentation. The authors of MySQL hereby grant you an additional
// permission to link the program and your derivative works with the
// separately licensed software that they have either included with
// the program or referenced in the documentation.
//
// Without limiting anything contained in the foregoing, this file,
// which is part of Connector/ODBC, is also subject to the
// Universal FOSS Exception, version 1.0, a copy of which can be found at
// https://oss.oracle.com/licenses/universal-foss-exception.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License, version 2.0, for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

static char *ui_xml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" \
"<interface>\n" \
  "<requires lib=\"gtk+\" version=\"2.24\"/>\n" \
  "<!-- interface-naming-policy toplevel-contextual -->\n" \
  "<object class=\"GtkAdjustment\" id=\"adjustment1\">\n" \
    "<property name=\"upper\">65535</property>\n" \
    "<property name=\"value\">3306</property>\n" \
    "<property name=\"step_increment\">1</property>\n" \
    "<property name=\"page_increment\">10</property>\n" \
  "</object>\n" \
  "<object class=\"GtkAdjustment\" id=\"adjustment2\">\n" \
    "<property name=\"upper\">1000000</property>\n" \
    "<property name=\"value\">100</property>\n" \
    "<property name=\"step_increment\">1</property>\n" \
    "<property name=\"page_increment\">10</property>\n" \
  "</object>\n" \
  "<object class=\"GtkWindow\" id=\"odbcdialog\">\n" \
    "<property name=\"width_request\">568</property>\n" \
    "<property name=\"visible\">True</property>\n" \
    "<property name=\"can_focus\">False</property>\n" \
    "<property name=\"title\">DES Connector/ODBC Data Source</property>\n" \
    "<property name=\"resizable\">False</property>\n" \
    "<property name=\"modal\">True</property>\n" \
    "<property name=\"window_position\">center-on-parent</property>\n" \
    "<property name=\"destroy_with_parent\">True</property>\n" \
    "<child>\n" \
      "<object class=\"GtkVBox\" id=\"vbox1\">\n" \
        "<property name=\"visible\">True</property>\n" \
        "<property name=\"can_focus\">False</property>\n" \
        "<child>\n" \
          "<object class=\"GtkImage\" id=\"header\">\n" \
            "<property name=\"width_request\">568</property>\n" \
            "<property name=\"height_request\">63</property>\n" \
            "<property name=\"visible\">True</property>\n" \
            "<property name=\"can_focus\">False</property>\n" \
          "</object>\n" \
          "<packing>\n" \
            "<property name=\"expand\">False</property>\n" \
            "<property name=\"fill\">True</property>\n" \
            "<property name=\"position\">0</property>\n" \
          "</packing>\n" \
        "</child>\n" \
        "<child>\n" \
          "<object class=\"GtkVBox\" id=\"vbox2\">\n" \
            "<property name=\"visible\">True</property>\n" \
            "<property name=\"can_focus\">False</property>\n" \
            "<property name=\"border_width\">12</property>\n" \
            "<property name=\"spacing\">8</property>\n" \
            "<child>\n" \
              "<object class=\"GtkFrame\" id=\"frame1\">\n" \
                "<property name=\"visible\">True</property>\n" \
                "<property name=\"can_focus\">False</property>\n" \
                "<property name=\"label_xalign\">0</property>\n" \
                "<child>\n" \
                  "<object class=\"GtkTable\" id=\"table1\">\n" \
                    "<property name=\"visible\">True</property>\n" \
                    "<property name=\"can_focus\">False</property>\n" \
                    "<property name=\"border_width\">8</property>\n" \
                    "<property name=\"n_rows\">4</property>\n" \
                    "<property name=\"n_columns\">4</property>\n" \
                    "<property name=\"column_spacing\">8</property>\n" \
                    "<property name=\"row_spacing\">8</property>\n" \
                    "<child>\n" \
                      "<object class=\"GtkLabel\" id=\"label2\">\n" \
                        "<property name=\"visible\">True</property>\n" \
                        "<property name=\"can_focus\">False</property>\n" \
                        "<property name=\"xalign\">1</property>\n" \
                        "<property name=\"label\" translatable=\"yes\">Data Source Name:</property>\n" \
                      "</object>\n" \
                      "<packing>\n" \
                        "<property name=\"x_options\">GTK_FILL</property>\n" \
                        "<property name=\"y_options\"/>\n" \
                      "</packing>\n" \
                    "</child>\n" \
                    "<child>\n" \
                      "<object class=\"GtkLabel\" id=\"label3\">\n" \
                        "<property name=\"visible\">True</property>\n" \
                        "<property name=\"can_focus\">False</property>\n" \
                        "<property name=\"xalign\">1</property>\n" \
                        "<property name=\"label\" translatable=\"yes\">Description:</property>\n" \
                      "</object>\n" \
                      "<packing>\n" \
                        "<property name=\"top_attach\">1</property>\n" \
                        "<property name=\"bottom_attach\">2</property>\n" \
                        "<property name=\"x_options\">GTK_FILL</property>\n" \
                        "<property name=\"y_options\"/>\n" \
                      "</packing>\n" \
                    "</child>\n" \
                    "<child>\n" \
                      "<object class=\"GtkEntry\" id=\"DSN\">\n" \
                        "<property name=\"visible\">True</property>\n" \
                        "<property name=\"can_focus\">True</property>\n" \
                        "<property name=\"tooltip_text\" translatable=\"yes\">An unique name for this data source</property>\n" \
                        "<property name=\"invisible_char\">●</property>\n" \
                        "<property name=\"primary_icon_activatable\">False</property>\n" \
                        "<property name=\"secondary_icon_activatable\">False</property>\n" \
                        "<property name=\"primary_icon_sensitive\">True</property>\n" \
                        "<property name=\"secondary_icon_sensitive\">True</property>\n" \
                      "</object>\n" \
                      "<packing>\n" \
                        "<property name=\"left_attach\">1</property>\n" \
                        "<property name=\"right_attach\">4</property>\n" \
                        "<property name=\"y_options\"/>\n" \
                      "</packing>\n" \
                    "</child>\n" \
                    "<child>\n" \
                      "<object class=\"GtkEntry\" id=\"DESCRIPTION\">\n" \
                        "<property name=\"visible\">True</property>\n" \
                        "<property name=\"can_focus\">True</property>\n" \
                        "<property name=\"tooltip_text\" translatable=\"yes\">A brief description for this data source</property>\n" \
                        "<property name=\"invisible_char\">●</property>\n" \
                        "<property name=\"primary_icon_activatable\">False</property>\n" \
                        "<property name=\"secondary_icon_activatable\">False</property>\n" \
                        "<property name=\"primary_icon_sensitive\">True</property>\n" \
                        "<property name=\"secondary_icon_sensitive\">True</property>\n" \
                      "</object>\n" \
                      "<packing>\n" \
                        "<property name=\"left_attach\">1</property>\n" \
                        "<property name=\"right_attach\">4</property>\n" \
                        "<property name=\"top_attach\">1</property>\n" \
                        "<property name=\"bottom_attach\">2</property>\n" \
                        "<property name=\"y_options\"/>\n" \
                      "</packing>\n" \
                    "</child>\n" \
                    "<child>\n" \
                      "<object class=\"GtkLabel\" id=\"label4\">\n" \
                        "<property name=\"visible\">True</property>\n" \
                        "<property name=\"can_focus\">False</property>\n" \
                        "<property name=\"xalign\">1</property>\n" \
                        "<property name=\"label\" translatable=\"yes\">DES executable path:</property>\n" \
                      "</object>\n" \
                      "<packing>\n" \
                        "<property name=\"top_attach\">2</property>\n" \
                        "<property name=\"bottom_attach\">3</property>\n" \
                        "<property name=\"x_options\">GTK_FILL</property>\n" \
                        "<property name=\"y_options\"/>\n" \
                      "</packing>\n" \
                    "</child>\n" \
                    "<child>\n" \
                      "<object class=\"GtkEntry\" id=\"DES_EXEC\">\n" \
                        "<property name=\"visible\">True</property>\n" \
                        "<property name=\"can_focus\">True</property>\n" \
                        "<property name=\"tooltip_text\" translatable=\"yes\">The path of the DES executable</property>\n" \
                        "<property name=\"invisible_char\">●</property>\n" \
                        "<property name=\"invisible_char_set\">True</property>\n" \
                        "<property name=\"primary_icon_activatable\">False</property>\n" \
                        "<property name=\"secondary_icon_activatable\">False</property>\n" \
                        "<property name=\"primary_icon_sensitive\">True</property>\n" \
                        "<property name=\"secondary_icon_sensitive\">True</property>\n" \
                      "</object>\n" \
                      "<packing>\n" \
                        "<property name=\"left_attach\">1</property>\n" \
                        "<property name=\"right_attach\">3</property>\n" \
                        "<property name=\"top_attach\">2</property>\n" \
                        "<property name=\"bottom_attach\">3</property>\n" \
                        "<property name=\"y_options\"/>\n" \
                      "</packing>\n" \
                    "</child>\n" \
                    "<child>\n" \
                      "<object class=\"GtkButton\" id=\"browse_exec\">\n" \
                        "<property name=\"label\" translatable=\"yes\">_Browse</property>\n" \
                        "<property name=\"visible\">True</property>\n" \
                        "<property name=\"can_focus\">True</property>\n" \
                        "<property name=\"receives_default\">False</property>\n" \
                        "<property name=\"use_underline\">True</property>\n" \
                      "</object>\n" \
                      "<packing>\n" \
                        "<property name=\"left_attach\">3</property>\n" \
                        "<property name=\"right_attach\">4</property>\n" \
                        "<property name=\"top_attach\">2</property>\n" \
                        "<property name=\"bottom_attach\">3</property>\n" \
                      "</packing>\n" \
                    "</child>\n" \
                    "<child>\n" \
                      "<object class=\"GtkLabel\" id=\"label5\">\n" \
                        "<property name=\"visible\">True</property>\n" \
                        "<property name=\"can_focus\">False</property>\n" \
                        "<property name=\"xalign\">1</property>\n" \
                        "<property name=\"label\" translatable=\"yes\">DES working directory:</property>\n" \
                      "</object>\n" \
                      "<packing>\n" \
                        "<property name=\"top_attach\">3</property>\n" \
                        "<property name=\"bottom_attach\">4</property>\n" \
                        "<property name=\"x_options\">GTK_FILL</property>\n" \
                        "<property name=\"y_options\"/>\n" \
                      "</packing>\n" \
                    "</child>\n" \
                    "<child>\n" \
                      "<object class=\"GtkEntry\" id=\"DES_WORKING_DIR\">\n" \
                        "<property name=\"visible\">True</property>\n" \
                        "<property name=\"can_focus\">True</property>\n" \
                        "<property name=\"tooltip_text\" translatable=\"yes\">The working directory specified for DES</property>\n" \
                        "<property name=\"invisible_char\">●</property>\n" \
                        "<property name=\"invisible_char_set\">True</property>\n" \
                        "<property name=\"primary_icon_activatable\">False</property>\n" \
                        "<property name=\"secondary_icon_activatable\">False</property>\n" \
                        "<property name=\"primary_icon_sensitive\">True</property>\n" \
                        "<property name=\"secondary_icon_sensitive\">True</property>\n" \
                      "</object>\n" \
                      "<packing>\n" \
                        "<property name=\"left_attach\">1</property>\n" \
                        "<property name=\"right_attach\">3</property>\n" \
                        "<property name=\"top_attach\">3</property>\n" \
                        "<property name=\"bottom_attach\">4</property>\n" \
                        "<property name=\"y_options\"/>\n" \
                      "</packing>\n" \
                    "</child>\n" \
                    "<child>\n" \
                      "<object class=\"GtkButton\" id=\"browse_dir\">\n" \
                        "<property name=\"label\" translatable=\"yes\">_Browse</property>\n" \
                        "<property name=\"visible\">True</property>\n" \
                        "<property name=\"can_focus\">True</property>\n" \
                        "<property name=\"receives_default\">False</property>\n" \
                        "<property name=\"use_underline\">True</property>\n" \
                      "</object>\n" \
                      "<packing>\n" \
                        "<property name=\"left_attach\">3</property>\n" \
                        "<property name=\"right_attach\">4</property>\n" \
                        "<property name=\"top_attach\">3</property>\n" \
                        "<property name=\"bottom_attach\">4</property>\n" \
                      "</packing>\n" \
                    "</child>\n" \
                  "</object>\n" \
                "</child>\n" \
                "<child type=\"label\">\n" \
                  "<object class=\"GtkLabel\" id=\"label1\">\n" \
                    "<property name=\"visible\">True</property>\n" \
                    "<property name=\"can_focus\">False</property>\n" \
                    "<property name=\"label\" translatable=\"yes\">ODBC Connection Parameters</property>\n" \
                  "</object>\n" \
                "</child>\n" \
              "</object>\n" \
              "<packing>\n" \
                "<property name=\"expand\">False</property>\n" \
                "<property name=\"fill\">True</property>\n" \
                "<property name=\"position\">0</property>\n" \
              "</packing>\n" \
            "</child>\n" \
            "<child>\n" \
              "<object class=\"GtkHBox\" id=\"hbox1\">\n" \
                "<property name=\"visible\">True</property>\n" \
                "<property name=\"can_focus\">False</property>\n" \
                "<property name=\"spacing\">8</property>\n" \
                "<property name=\"homogeneous\">True</property>\n" \
                "<child>\n" \
                  "<object class=\"GtkFixed\" id=\"fixed1\">\n" \
                    "<property name=\"visible\">True</property>\n" \
                    "<property name=\"can_focus\">False</property>\n" \
                  "</object>\n" \
                  "<packing>\n" \
                    "<property name=\"expand\">True</property>\n" \
                    "<property name=\"fill\">True</property>\n" \
                    "<property name=\"position\">0</property>\n" \
                  "</packing>\n" \
                "</child>\n" \
                "<child>\n" \
                  "<object class=\"GtkFixed\" id=\"fixed2\">\n" \
                    "<property name=\"visible\">True</property>\n" \
                    "<property name=\"can_focus\">False</property>\n" \
                  "</object>\n" \
                  "<packing>\n" \
                    "<property name=\"expand\">True</property>\n" \
                    "<property name=\"fill\">True</property>\n" \
                    "<property name=\"position\">1</property>\n" \
                  "</packing>\n" \
                "</child>\n" \
                "<child>\n" \
                  "<object class=\"GtkFixed\" id=\"fixed3\">\n" \
                    "<property name=\"visible\">True</property>\n" \
                    "<property name=\"can_focus\">False</property>\n" \
                  "</object>\n" \
                  "<packing>\n" \
                    "<property name=\"expand\">True</property>\n" \
                    "<property name=\"fill\">True</property>\n" \
                    "<property name=\"position\">2</property>\n" \
                  "</packing>\n" \
                "</child>\n" \
                "<child>\n" \
                  "<object class=\"GtkButton\" id=\"help\">\n" \
                    "<property name=\"label\">_Help</property>\n" \
                    "<property name=\"can_focus\">True</property>\n" \
                    "<property name=\"receives_default\">False</property>\n" \
                    "<property name=\"use_underline\">True</property>\n" \
                  "</object>\n" \
                  "<packing>\n" \
                    "<property name=\"expand\">False</property>\n" \
                    "<property name=\"fill\">True</property>\n" \
                    "<property name=\"pack_type\">end</property>\n" \
                    "<property name=\"position\">3</property>\n" \
                  "</packing>\n" \
                "</child>\n" \
                "<child>\n" \
                  "<object class=\"GtkButton\" id=\"cancel\">\n" \
                    "<property name=\"label\" translatable=\"yes\">_Cancel</property>\n" \
                    "<property name=\"visible\">True</property>\n" \
                    "<property name=\"can_focus\">True</property>\n" \
                    "<property name=\"receives_default\">False</property>\n" \
                    "<property name=\"use_underline\">True</property>\n" \
                  "</object>\n" \
                  "<packing>\n" \
                    "<property name=\"expand\">False</property>\n" \
                    "<property name=\"fill\">True</property>\n" \
                    "<property name=\"pack_type\">end</property>\n" \
                    "<property name=\"position\">4</property>\n" \
                  "</packing>\n" \
                "</child>\n" \
                "<child>\n" \
                  "<object class=\"GtkButton\" id=\"ok\">\n" \
                    "<property name=\"label\" translatable=\"yes\">_OK</property>\n" \
                    "<property name=\"visible\">True</property>\n" \
                    "<property name=\"can_focus\">True</property>\n" \
                    "<property name=\"receives_default\">False</property>\n" \
                    "<property name=\"use_underline\">True</property>\n" \
                  "</object>\n" \
                  "<packing>\n" \
                    "<property name=\"expand\">False</property>\n" \
                    "<property name=\"fill\">True</property>\n" \
                    "<property name=\"pack_type\">end</property>\n" \
                    "<property name=\"position\">5</property>\n" \
                  "</packing>\n" \
                "</child>\n" \
              "</object>\n" \
              "<packing>\n" \
                "<property name=\"expand\">False</property>\n" \
                "<property name=\"fill\">True</property>\n" \
                "<property name=\"position\">1</property>\n" \
              "</packing>\n" \
            "</child>\n" \
          "</object>\n" \
          "<packing>\n" \
            "<property name=\"expand\">True</property>\n" \
            "<property name=\"fill\">True</property>\n" \
            "<property name=\"position\">1</property>\n" \
          "</packing>\n" \
        "</child>\n" \
      "</object>\n" \
    "</child>\n" \
  "</object>\n" \
"</interface>\n";