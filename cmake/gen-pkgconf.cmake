# Always generate a relocatable pkg-config, see https://bugs.freedesktop.org/show_bug.cgi?id=62018.
set(RVMI_PKGCONF_INSTALL_PATH "${CMAKE_INSTALL_LIBDIR}/pkgconfig")

cmake_path(RELATIVE_PATH CMAKE_INSTALL_INCLUDEDIR BASE_DIRECTORY "${RVMI_PKGCONF_INSTALL_PATH}"
           OUTPUT_VARIABLE RVMI_PKGCONF_REL_INCLUDEDIR)

set(RVMI_PKGCONF_INCLUDEDIR "\${pcfiledir}/${RVMI_PKGCONF_REL_INCLUDEDIR}")
set(RVMI_PKGCONF_LIBDIR "\${pcfiledir}/..")

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/rvmi.pc.in" "${CMAKE_CURRENT_BINARY_DIR}/rvmi.pc"
               @ONLY)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/rvmi.pc" DESTINATION "${RVMI_PKGCONF_INSTALL_PATH}")
