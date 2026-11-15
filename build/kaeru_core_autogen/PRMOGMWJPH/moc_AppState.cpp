/****************************************************************************
** Meta object code from reading C++ file 'AppState.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.2.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../src/core/AppState.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'AppState.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.2.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_kaeru__AppState_t {
    const uint offsetsAndSize[18];
    char stringdata0[80];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_kaeru__AppState_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_kaeru__AppState_t qt_meta_stringdata_kaeru__AppState = {
    {
QT_MOC_LITERAL(0, 15), // "kaeru::AppState"
QT_MOC_LITERAL(16, 7), // "changed"
QT_MOC_LITERAL(24, 0), // ""
QT_MOC_LITERAL(25, 13), // "moduleChanged"
QT_MOC_LITERAL(39, 6), // "Module"
QT_MOC_LITERAL(46, 1), // "m"
QT_MOC_LITERAL(48, 7), // "enabled"
QT_MOC_LITERAL(56, 16), // "pausedAllChanged"
QT_MOC_LITERAL(73, 6) // "paused"

    },
    "kaeru::AppState\0changed\0\0moduleChanged\0"
    "Module\0m\0enabled\0pausedAllChanged\0"
    "paused"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_kaeru__AppState[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       3,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   32,    2, 0x06,    1 /* Public */,
       3,    2,   33,    2, 0x06,    2 /* Public */,
       7,    1,   38,    2, 0x06,    5 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 4, QMetaType::Bool,    5,    6,
    QMetaType::Void, QMetaType::Bool,    8,

       0        // eod
};

void kaeru::AppState::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<AppState *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->changed(); break;
        case 1: _t->moduleChanged((*reinterpret_cast< std::add_pointer_t<Module>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[2]))); break;
        case 2: _t->pausedAllChanged((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (AppState::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&AppState::changed)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (AppState::*)(Module , bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&AppState::moduleChanged)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (AppState::*)(bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&AppState::pausedAllChanged)) {
                *result = 2;
                return;
            }
        }
    }
}

const QMetaObject kaeru::AppState::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_kaeru__AppState.offsetsAndSize,
    qt_meta_data_kaeru__AppState,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_kaeru__AppState_t
, QtPrivate::TypeAndForceComplete<AppState, std::true_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<Module, std::false_type>, QtPrivate::TypeAndForceComplete<bool, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<bool, std::false_type>



>,
    nullptr
} };


const QMetaObject *kaeru::AppState::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *kaeru::AppState::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_kaeru__AppState.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int kaeru::AppState::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 3)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 3;
    }
    return _id;
}

// SIGNAL 0
void kaeru::AppState::changed()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void kaeru::AppState::moduleChanged(Module _t1, bool _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void kaeru::AppState::pausedAllChanged(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
