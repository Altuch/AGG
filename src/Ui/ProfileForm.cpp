#include "Ui/ProfileForm.h"
#include "Ui/FormNavigator.h"

using namespace Osp::Base;
using namespace Osp::Ui;
using namespace Osp::Ui::Controls;

ProfileForm::ProfileForm(void) : pConnection(null), pProfileText(null) {}

ProfileForm::~ProfileForm(void) {
    if (pConnection != null) pConnection->SetProfileListener(null);
}

result ProfileForm::Initialize(AggConnection* pConn) {
    pConnection = pConn;
    return Construct(L"IDF_PROFILE");
}

result ProfileForm::OnInitializing(void) {
    SetTitleText(L"Мій профіль");

    SetSoftkeyActionId(SOFTKEY_0, ID_SOFTKEY_BACK);
    AddSoftkeyActionListener(SOFTKEY_0, *this);

    pProfileText = static_cast<EditArea*>(GetControl(L"IDC_PROFILE_TEXT"));
    if (pProfileText != null) pProfileText->SetKeypadEnabled(false);

    String initial;
    initial.Append(L"Email: ");
    initial.Append(pConnection != null ? pConnection->GetLogin() : String(L""));
    initial.Append(L"\n");

    String nickname = pConnection != null ? pConnection->GetNickname() : String(L"");
    if (!nickname.IsEmpty()) {
        initial.Append(L"Нік: ");
        initial.Append(nickname);
        initial.Append(L"\n");
    }

    initial.Append(L"\nЗавантаження анкети...");
    ShowText(initial);

    if (pConnection != null) {
        pConnection->SetProfileListener(this);
        pConnection->RequestOwnProfile();
    }

    return E_SUCCESS;
}

result ProfileForm::OnTerminating(void) {
    return E_SUCCESS;
}

void ProfileForm::ShowText(const String& text) {
    if (pProfileText == null) return;

    pProfileText->SetText(text);
    if (GetParent() != null) {
        pProfileText->RequestRedraw(true);
        pProfileText->Draw();
        pProfileText->Show();
    }
}

void ProfileForm::AppendField(String& out, const String& label, const String& value) const {
    if (value.IsEmpty()) return;
    out.Append(label);
    out.Append(L": ");
    out.Append(value);
    out.Append(L"\n");
}

void ProfileForm::OnProfileReceived(const ProfileInfo& info) {
    String text;
    text.Append(L"Email: ");
    text.Append(pConnection != null ? pConnection->GetLogin() : String(L""));
    text.Append(L"\n");

    String nickname = pConnection != null ? pConnection->GetNickname() : String(L"");
    AppendField(text, L"Нік", nickname);

    String fullName = info.firstName;
    if (!info.lastName.IsEmpty()) {
        if (!fullName.IsEmpty()) fullName.Append(L" ");
        fullName.Append(info.lastName);
    }
    AppendField(text, L"Ім'я", fullName);
    AppendField(text, L"Місто", info.location);
    AppendField(text, L"Дата народження", info.birthday);
    AppendField(text, L"Знак зодіаку", info.zodiac);
    AppendField(text, L"Телефон", info.phone);

    if (info.sex == L"1") AppendField(text, L"Стать", String(L"Чоловіча"));
    else if (info.sex == L"2") AppendField(text, L"Стать", String(L"Жіноча"));

    // Сервер (mrimsu/mrim-server) уміє лише повертати анкету, а не
    // редагувати - якщо всі поля вище відсутні, це не помилка: просто
    // нікому було їх заповнити.
    if (fullName.IsEmpty() && info.location.IsEmpty() && info.birthday.IsEmpty()
        && info.zodiac.IsEmpty() && info.phone.IsEmpty() && info.sex.IsEmpty()) {
        text.Append(L"\nСервер не має додаткових даних анкети.");
    }

    ShowText(text);
}

void ProfileForm::OnProfileNotFound(void) {
    String text;
    text.Append(L"Email: ");
    text.Append(pConnection != null ? pConnection->GetLogin() : String(L""));
    text.Append(L"\n");

    String nickname = pConnection != null ? pConnection->GetNickname() : String(L"");
    AppendField(text, L"Нік", nickname);

    text.Append(L"\nСервер не знайшов анкету для цього акаунту.");
    ShowText(text);
}

void ProfileForm::OnActionPerformed(const Control& source, int actionId) {
    switch (actionId) {
        case ID_SOFTKEY_BACK: {
            AggConnection* pConn = pConnection;
            if (pConn != null) pConn->SetProfileListener(null);
            pConnection = null;

            FormNavigator::GoToContactList(pConn, this);
            break;
        }

        default:
            break;
    }
}
