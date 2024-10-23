#pragma once
class ImGuiBase
{
protected:
    virtual void fileMainMenu();

    virtual void viewMainMenu();

    virtual void customMainMenu();

    virtual void customUI();

private:
    friend class ImGuiState;
};

