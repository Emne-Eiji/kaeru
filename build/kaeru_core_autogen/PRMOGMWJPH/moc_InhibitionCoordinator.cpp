/****************************************************************************
** Meta object code from reading C++ file 'InhibitionCoordinator.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.2.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../src/core/InhibitionCoordinator.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'InhibitionCoordinator.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.2.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_kaeru__InhibitionCoordinator_t {
    const uint offsetsAndSize[28];
    char stringdata0[167];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_kaeru__InhibitionCoordinator_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_kaeru__InhibitionCoordinator_t qt_meta_stringdata_kaeru__InhibitionCoordinator = {
    {
QT_MOC_LITERAL(0, 28), // "kaeru::InhibitionCoordinator"
QT_MOC_LITERAL(29, 12), // "stateChanged"
QT_MOC_LITERAL(42, 0), // ""
QT_MOC_LITERAL(43, 7), // "expired"
QT_MOC_LITERAL(51, 9), // "requestId"
QT_MOC_LITERAL(61, 17), // "InhibitionRequest"
QT_MOC_LITERAL(79, 7), // "request"
QT_MOC_LITERAL(87, 18), // "onBindingActivated"
QT_MOC_LITERAL(106, 9), // "bindingId"
QT_MOC_LITERAL(116, 15), // "onBindingFailed"
QT_MOC_LITERAL(132, 6), // "denied"
QT_MOC_LITERAL(139, 6), // "detail"
QT_MOC_LITERAL(146, 13), // "onBindingLost"
QT_MOC_LITERAL(160, 6) // "reason"

    },
    "kaeru::InhibitionCoordinator\0stateChanged\0"
    "\0expired\0requestId\0InhibitionRequest\0"
    "request\0onBindingActivated\0bindingId\0"
    "onBindingFailed\0denied\0detail\0"
    "onBindingLost\0reason"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_kaeru__InhibitionCoordinator[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   44,    2, 0x06,    1 /* Public */,
       3,    2,   45,    2, 0x06,    2 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       7,    1,   50,    2, 0x08,    5 /* Private */,
       9,    3,   53,    2, 0x08,    7 /* Private */,
      12,    2,   60,    2, 0x08,   11 /* Private */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void, QMetaType::QUuid, 0x80000000 | 5,    4,    6,

 // slots: parameters
    QMetaType::Void, QMetaType::QString,    8,
    QMetaType::Void, QMetaType::QString, QMetaType::Bool, QMetaType::QString,    8,   10,   11,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,    8,   13,

       0        // eod
};

void kaeru::InhibitionCoordinator::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<InhibitionCoordinator *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->stateChanged(); break;
        case 1: _t->expired((*reinterpret_cast< std::add_pointer_t<QUuid>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<InhibitionRequest>>(_a[2]))); break;
        case 2: _t->onBindingActivated((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 3: _t->onBindingFailed((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[3]))); break;
        case 4: _t->onBindingLost((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (InhibitionCoordinator::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&InhibitionCoordinator::stateChanged)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (InhibitionCoordinator::*)(const QUuid & , const InhibitionRequest & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&InhibitionCoordinator::expired)) {
                *result = 1;
                return;
            }
        }
    }
}

const QMetaObject kaeru::InhibitionCoordinator::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_kaeru__InhibitionCoordinator.offsetsAndSize,
    qt_meta_data_kaeru__InhibitionCoordinator,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_kaeru__InhibitionCoordinator_t
, QtPrivate::TypeAndForceComplete<InhibitionCoordinator, std::true_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QUuid &, std::false_type>, QtPrivate::TypeAndForceComplete<const InhibitionRequest &, std::false_type>
, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QString &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QString &, std::false_type>, QtPrivate::TypeAndForceComplete<bool, std::false_type>, QtPrivate::TypeAndForceComplete<const QString &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QString &, std::false_type>, QtPrivate::TypeAndForceComplete<const QString &, std::false_type>


>,
    nullptr
} };


const QMetaObject *kaeru::InhibitionCoordinator::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *kaeru::InhibitionCoordinator::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_kaeru__InhibitionCoordinator.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int kaeru::InhibitionCoordinator::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 5)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 5;
    }
    return _id;
}

// SIGNAL 0
void kaeru::InhibitionCoordinator::stateChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void kaeru::InhibitionCoordinator::expired(const QUuid & _t1, const InhibitionRequest & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
