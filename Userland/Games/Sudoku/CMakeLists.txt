serenity_component(
    Sudoku
    RECOMMENDED
    TARGETS Sudoku
)

set(SOURCES
    BoardWidget.cpp
    Game.cpp
    main.cpp
)

serenity_app(Sudoku ICON app-sudoku)
target_link_libraries(Sudoku PRIVATE LibCore LibGfx LibGUI LibConfig LibMain LibDesktop LibURL)
