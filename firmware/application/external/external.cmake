# firmware/application/external/external.cmake
# Minimalny zestaw external apps: budujemy WYŁĄCZNIE TPMS RX,
# żeby nie przepełnić flasha aplikacji (M0).

set(EXTCPPSRC

    # tpmsrx
    external/tpmsrx/main.cpp
    external/tpmsrx/tpms_app.cpp
)

set(EXTAPPLIST
    tpmsrx
)
