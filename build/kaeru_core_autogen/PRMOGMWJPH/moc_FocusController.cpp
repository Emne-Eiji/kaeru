/****************************************************************************
** Meta object code from reading C++ file 'FocusController.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.2.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../src/core/FocusController.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'FocusController.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.2.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_kaeru__FocusController_t {
    const uint offsetsAndSize[22];
    char stringdata0[130];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_kaeru__FocusController_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_kaeru__FocusController_t qt_meta_stringdata_kaeru__FocusController = {
    {
QT_MOC_LITERAL(0, 22), // "kaeru::FocusController"
QT_MOC_LITERAL(23, 12), // "stateChanged"
QT_MOC_LITERAL(36, 0), // ""
QT_MOC_LITERAL(37, 17), // "intervalCompleted"
QT_MOC_LITERAL(55, 5), // "Phase"
QT_MOC_LITERAL(61, 13), // "finishedPhase"
QT_MOC_LITERAL(75, 9), // "plannedMs"
QT_MOC_LITERAL(85, 10), // "achievedMs"
QT_MOC_LITERAL(96, 15), // "intervalStarted"
QT_MOC_LITERAL(112, 5), // "phase"
QT_MOC_LITERAL(118, 11) // "stoppedIdle"

    },
    "kaeru::FocusController\0stateChanged\0"
    "\0intervalCompleted\0Phase\0finishedPhase\0"
    "plannedMs\0achievedMs\0intervalStarted\0"
    "phase\0stoppedIdle"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_kaeru__FocusController[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       4,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   38,    2, 0x06,    1 /* Public */,
       3,    3,   39,    2, 0x06,    2 /* Public */,
       8,    1,   46,    2, 0x06,    6 /* Public */,
      10,    0,   49,    2, 0x06,    8 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 4, QMetaType::LongLong, QMetaType::LongLong,    5,    6,    7,
    QMetaType::Void, 0x80000000 | 4,    9,
    QMetaType::Void,

       0        // eod
};

void kaeru::FocusController::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<FocusController *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->stateChanged(); break;
        case 1: _t->intervalCompleted((*reinterpret_cast< std::add_pointer_t<Phase>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<qint64>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<qint64>>(_a[3]))); break;
        case 2: _t->intervalStarted((*reinterpret_cast< std::add_pointer_t<Phase>>(_a[1]))); break;
        case 3: _t->stoppedIdle(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (FocusController::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&FocusController::stateChanged)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (FocusController::*)(Phase , qint64 , qint64 );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&FocusController::intervalCompleted)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (FocusController::*)(Phase );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&FocusController::intervalStarted)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (FocusController::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&FocusController::stoppedIdle)) {
                *result = 3;
                return;
            }
        }
    }
}

const QMetaObject kaeru::FocusController::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_kaeru__FocusController.offsetsAndSize,
    qt_meta_data_kaeru__FocusController,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_kaeru__FocusController_t
, QtPrivate::TypeAndForceComplete<FocusController, std::true_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<Phase, std::false_type>, QtPrivate::TypeAndForceComplete<qint64, std::false_type>, QtPrivate::TypeAndForceComplete<qint64, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<Phase, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>



>,
    nullptr
} };


const QMetaObject *kaeru::FocusController::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *kaeru::FocusController::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_kaeru__FocusController.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int kaeru::FocusController::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 4)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 4;
    }
    return _id;
}

// SIGNAL 0
void kaeru::FocusController::stateChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void kaeru::FocusController::intervalCompleted(Phase _t1, qint64 _t2, qint64 _t3)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void kaeru::FocusController::intervalStarted(Phase _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void kaeru::FocusController::stoppedIdle()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
