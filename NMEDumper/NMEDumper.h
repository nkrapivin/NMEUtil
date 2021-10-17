// Приведенный ниже блок ifdef — это стандартный метод создания макросов, упрощающий процедуру
// экспорта из библиотек DLL. Все файлы данной DLL скомпилированы с использованием символа NMEDUMPER_EXPORTS
// Символ, определенный в командной строке. Этот символ не должен быть определен в каком-либо проекте,
// использующем данную DLL. Благодаря этому любой другой проект, исходные файлы которого включают данный файл, видит
// функции NMEDUMPER_API как импортированные из DLL, тогда как данная DLL видит символы,
// определяемые данным макросом, как экспортированные.
#include "pch.h"
#ifdef NMEDUMPER_EXPORTS
#define NMEDUMPER_API __declspec(dllexport)
#else
#define NMEDUMPER_API __declspec(dllimport)
#endif

#define NMEDUMPER_X EXTERN_C NMEDUMPER_API

NMEDUMPER_X int luaopen_NMEDumper(void* pL);
