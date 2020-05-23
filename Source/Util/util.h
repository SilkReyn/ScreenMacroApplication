#pragma once

#include<stdexcept>
#include<QtGlobal>


#ifndef NDEBUG  // if debug build
    #include <QElapsedTimer>
    #include <QtDebug>  // extended QDebug syntax

    #define DBG_(msg) qDebug() << msg
    #define BENCHMARK_(iterations, expr){\
        QElapsedTimer timer; \
        timer.start(); \
        for(int i=1; i<=iterations; i++){expr;} \
        DBG_(#expr << "took" << (timer.elapsed()/iterations) << "ms.");}
#else
    #define DBG_(msg)
    #define BENCHMARK_(iterations, expr) expr
#endif

#define DEL_PTR_(ptr) if (ptr) delete ptr

