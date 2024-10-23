#pragma once

#include <memory>
#include <string>

enum BUTTON
{
	BUTTON_LEFT,
	BUTTON_RIGHT,
	BUTTON_MIDDLE,
	BUTTON_MAX_BUTTONS
};

#define DEFINE_KEY(name, code) KEY_##name = code

enum KEY
{
    //GLFW KEY
    DEFINE_KEY(BACKSPACE, 0x103),
    DEFINE_KEY(ENTER, 0x101),
    DEFINE_KEY(TAB, 0x102),

    DEFINE_KEY(PAUSE, 0x11C),
    DEFINE_KEY(ESCAPE, 0x100),

    DEFINE_KEY(SPACE, 0x20),
    DEFINE_KEY(END, 0x10D),
    DEFINE_KEY(HOME, 0x10C),
    DEFINE_KEY(LEFT, 0x107),
    DEFINE_KEY(UP, 0x109),
    DEFINE_KEY(RIGHT, 0x106),
    DEFINE_KEY(DOWN, 0x108),

    DEFINE_KEY(INSERT, 0x104),
    DEFINE_KEY(DELETE, 0x105),

    /** @brief The 0 key */
    KEY_0 = 0x30,
    /** @brief The 1 key */
    KEY_1 = 0x31,
    /** @brief The 2 key */
    KEY_2 = 0x32,
    /** @brief The 3 key */
    KEY_3 = 0x33,
    /** @brief The 4 key */
    KEY_4 = 0x34,
    /** @brief The 5 key */
    KEY_5 = 0x35,
    /** @brief The 6 key */
    KEY_6 = 0x36,
    /** @brief The 7 key */
    KEY_7 = 0x37,
    /** @brief The 8 key */
    KEY_8 = 0x38,
    /** @brief The 9 key */
    KEY_9 = 0x39,

    DEFINE_KEY(A, 0x41),
    DEFINE_KEY(B, 0x42),
    DEFINE_KEY(C, 0x43),
    DEFINE_KEY(D, 0x44),
    DEFINE_KEY(E, 0x45),
    DEFINE_KEY(F, 0x46),
    DEFINE_KEY(G, 0x47),
    DEFINE_KEY(H, 0x48),
    DEFINE_KEY(I, 0x49),
    DEFINE_KEY(J, 0x4A),
    DEFINE_KEY(K, 0x4B),
    DEFINE_KEY(L, 0x4C),
    DEFINE_KEY(M, 0x4D),
    DEFINE_KEY(N, 0x4E),
    DEFINE_KEY(O, 0x4F),
    DEFINE_KEY(P, 0x50),
    DEFINE_KEY(Q, 0x51),
    DEFINE_KEY(R, 0x52),
    DEFINE_KEY(S, 0x53),
    DEFINE_KEY(T, 0x54),
    DEFINE_KEY(U, 0x55),
    DEFINE_KEY(V, 0x56),
    DEFINE_KEY(W, 0x57),
    DEFINE_KEY(X, 0x58),
    DEFINE_KEY(Y, 0x59),
    DEFINE_KEY(Z, 0x5A),

    //DEFINE_KEY(LWIN, 0x5B),
    //DEFINE_KEY(RWIN, 0x5C),
    //DEFINE_KEY(APPS, 0x5D),

    //DEFINE_KEY(SLEEP, 0x5F),

    //DEFINE_KEY(NUMPAD0, 0x60),
    //DEFINE_KEY(NUMPAD1, 0x61),
    //DEFINE_KEY(NUMPAD2, 0x62),
    //DEFINE_KEY(NUMPAD3, 0x63),
    //DEFINE_KEY(NUMPAD4, 0x64),
    //DEFINE_KEY(NUMPAD5, 0x65),
    //DEFINE_KEY(NUMPAD6, 0x66),
    //DEFINE_KEY(NUMPAD7, 0x67),
    //DEFINE_KEY(NUMPAD8, 0x68),
    //DEFINE_KEY(NUMPAD9, 0x69),
    //DEFINE_KEY(MULTIPLY, 0x6A),
    //DEFINE_KEY(ADD, 0x6B),
    //DEFINE_KEY(SEPARATOR, 0x6C),
    //DEFINE_KEY(SUBTRACT, 0x6D),
    //DEFINE_KEY(DECIMAL, 0x6E),
    //DEFINE_KEY(DIVIDE, 0x6F),
    //DEFINE_KEY(F1, 0x70),
    //DEFINE_KEY(F2, 0x71),
    //DEFINE_KEY(F3, 0x72),
    //DEFINE_KEY(F4, 0x73),
    //DEFINE_KEY(F5, 0x74),
    //DEFINE_KEY(F6, 0x75),
    //DEFINE_KEY(F7, 0x76),
    //DEFINE_KEY(F8, 0x77),
    //DEFINE_KEY(F9, 0x78),
    //DEFINE_KEY(F10, 0x79),
    //DEFINE_KEY(F11, 0x7A),
    //DEFINE_KEY(F12, 0x7B),
    //DEFINE_KEY(F13, 0x7C),
    //DEFINE_KEY(F14, 0x7D),
    //DEFINE_KEY(F15, 0x7E),
    //DEFINE_KEY(F16, 0x7F),
    //DEFINE_KEY(F17, 0x80),
    //DEFINE_KEY(F18, 0x81),
    //DEFINE_KEY(F19, 0x82),
    //DEFINE_KEY(F20, 0x83),
    //DEFINE_KEY(F21, 0x84),
    //DEFINE_KEY(F22, 0x85),
    //DEFINE_KEY(F23, 0x86),
    //DEFINE_KEY(F24, 0x87),

    //DEFINE_KEY(NUMLOCK, 0x90),
    //DEFINE_KEY(SCROLL, 0x91),

    //DEFINE_KEY(NUMPAD_EQUAL, 0x92),

    DEFINE_KEY(LSHIFT, 340),
    DEFINE_KEY(RSHIFT, 344),
    DEFINE_KEY(LCONTROL, 341),
    DEFINE_KEY(RCONTROL, 345),
    DEFINE_KEY(LALT, 342),
    DEFINE_KEY(RALT, 346),

    //DEFINE_KEY(SEMICOLON, 0xBA),
    //DEFINE_KEY(PLUS, 0xBB),
    //DEFINE_KEY(COMMA, 0xBC),
    //DEFINE_KEY(MINUS, 0xBD),
    //DEFINE_KEY(PERIOD, 0xBE),
    //DEFINE_KEY(SLASH, 0xBF),
    //DEFINE_KEY(GRAVE, 0xC0),

    //KEYS_MAX_KEYS
};

class InputManager
{
public:
    static void Init();
    static void Shutdown();
    static InputManager& GetInstance();
    void update(double _deltatime);
    bool GetIsKeyDown(KEY key);
    bool GetIsKeyUp(KEY key);
    bool GetWasKeyDown(KEY key);
    bool GetWasKeyUp(KEY key);
    bool GetIsButtonDown(BUTTON button);
    bool GetIsButtonUp(BUTTON button);
    bool GetWasButtonDown(BUTTON button);
    bool GetWasButtonUp(BUTTON button);
    void GetMousePos(int& x, int& y);
    void GetPrevMousePos(int& x, int& y);
    void ProcessKey(KEY key, bool pressed);
    void ProcessButton(BUTTON button, bool pressed);
    void ProcessMouseMove(short x, short y);
    void ProcessMouseWheel(signed char z_delta);

private:
    InputManager() {};
    static std::unique_ptr<InputManager> instance_;
};
