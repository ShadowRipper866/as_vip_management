## [AS] VIP Management
Модуль к плагину [admin_system](https://github.com/Pisex/cs2-admin_system/releases/latest), который позволяет управлять VIP статусами из игры
### Требования
---
- [Metamod:Source](https://www.sourcemm.net/downloads.php?branch=dev)
- [Utils](https://github.com/Pisex/cs2-menus/releases/latest)
- [AdminSystem](https://github.com/Pisex/cs2-admin_system/releases/latest)
- [VIP Core](https://github.com/Pisex/cs2-vip/releases/latest)
---
### Пример конфигурации
Путь: `addons/configs/admin_system/vip_admin.ini`
```ini
"Config"
{
    "times"
    { // Шаблон: "Название времени" "Фактическое время в секундах"
        "1 час"    "3600"
        "6 часов"   "21600" 
        "1 день"     "86400"
        "7 дней"    "604800"
        "30 дней"   "2592000"
    }

    "groups"
    {// Шаблон: "Название группы в меню" "Фактическое название, что используется для выдачи"
        "Отображение VIP"		"vip"
        "example3"	"example"
    }

    "accessFlag" "@admin/vip"
    "debug"     "0" // вкл/выкл отладночные сообщения

}
```
### Добавить в `translations/admin_system.phrases.txt`
```
    "Category_VIPManagement"
    {
        "ru"    "Управление VIP"
        "ua"    "Керування VIP"
        "en"    "VIP Management"
    }
	"Item_EditVip"
	{
		"ru"  "Изменить VIP"
		"ua"  "Змінити VIP"
		"en"  "Change VIP"
	}
    "Item_GiveVIP"
    {
        "ru"    "Выдать VIP"
        "ua"    "Видати VIP"
        "en"    "Give VIP"
    }
    "VIPManagement_SelectChangeMode"
    {
        "ru"    "Выберите режим смены времени"
        "ua"    "Оберіть режим зміни часу"
        "en"    "Select time change mode"
    }
    "VIPManagement_ReplaceMode"
    {
        "ru"    "Замена"
        "ua"    "Заміна"
        "en"    "Replace"
    }
    "VIPManagement_AddMode"
    {
        "ru"    "Добавить к текущему времени"
        "ua"    "Додати до поточного часу"
        "en"    "Add to current time"
    }
    "VIPManagement_GiveVIP"
    {
        "ru"    "Выдача VIP статуса"
        "ua"    "Видача VIP статусу"
        "en"    "Give VIP Status"
    }
    "VIPManagement_VIPGroup"
    {
        "ru"    "VIP Группа"
        "ua"    "VIP Група"
        "en"    "VIP Group"
    }
    "VIPManagement_SelectGroup"
    {
        "ru"    "Выберите группу"
        "ua"    "Оберіть групу"
        "en"    "Select Group"
    }
    "VIPManagement_VIPExpires"
    {
        "ru"    "Срок действия"
        "ua"    "Термін дії"
        "en"    "Duration"
    }
    "VIPManagement_SelectTime"
    {
        "ru"    "Выберите срок"
        "ua"    "Оберіть термін"
        "en"    "Select Duration"
    }
    "VIPManagement_Apply"
    {
        "ru"    "Подтвердить и выдать"
        "ua"    "Підтвердити та видати"
        "en"    "Confirm and Give"
    }
    "VIPManagement_SelectPlayer"
    {
        "ru"    "Выберите игрока"
        "ua"    "Оберіть гравця"
        "en"    "Select Player"
    }
    "VIPManagement_ChangeVIPPlayer"
    {
        "ru"    "Редактировать VIP"
        "ua"    "Редагувати VIP"
        "en"    "Edit VIP"
    }
    "VIPManagement_DeleteVIP"
    {
        "ru"    "Удалить VIP"
        "ua"    "Видалити VIP"
        "en"    "Remove VIP"
    }
    "VIPManagement_VIPExpiresDisabled"
    {
        "ru"    "Точное время"
        "ua"    "Точний час"
        "en"    "Exact time"
    }
    "VIPManagement_SelectVIPPlayer"
    {
        "ru"    "Выберите VIP игрока"
        "ua"    "Оберіть VIP гравця"
        "en"    "Select VIP player"
    }
```
### Добавить в `configs/admin_system/sorting.ini`
```
    "vip_management"
    {
        "item" "give_vip"
        "item" "edit_vip"
    }
```
