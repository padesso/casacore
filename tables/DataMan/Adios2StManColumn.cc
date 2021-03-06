//# Adios2StManColumn.cc: The Column of the ADIOS2 Storage Manager
//# Copyright (C) 2018
//# Associated Universities, Inc. Washington DC, USA.
//#
//# This library is free software; you can redistribute it and/or modify it
//# under the terms of the GNU Library General Public License as published by
//# the Free Software Foundation; either version 2 of the License, or (at your
//# option) any later version.
//#
//# This library is distributed in the hope that it will be useful, but WITHOUT
//# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
//# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
//# License for more details.
//#
//# You should have received a copy of the GNU Library General Public License
//# along with this library; if not, write to the Free Software Foundation,
//# Inc., 675 Massachusetts Ave, Cambridge, MA 02139, USA.
//#
//# Correspondence concerning AIPS++ should be addressed as follows:
//#        Internet email: aips2-request@nrao.edu.
//#        Postal address: AIPS++ Project Office
//#                        National Radio Astronomy Observatory
//#                        520 Edgemont Road
//#                        Charlottesville, VA 22903-2475 USA
//#
//# $Id$

#include "Adios2StManColumn.h"

namespace casacore
{

Adios2StManColumn::Adios2StManColumn(
        Adios2StMan::impl *aParent,
        int aDataType,
        String aColName,
        std::shared_ptr<adios2::IO> aAdiosIO)
    :StManColumn(aDataType),
    itsStManPtr(aParent),
    itsColumnName(aColName),
    itsAdiosIO(aAdiosIO)
{
}

String Adios2StManColumn::getColumnName()
{
    return itsColumnName;
}

void Adios2StManColumn::setShapeColumn(const IPosition &aShape)
{
    isShapeFixed = true;
    itsCasaShape = aShape;
    itsAdiosShape.resize(aShape.size() + 1);
    itsAdiosStart.resize(aShape.size() + 1);
    itsAdiosCount.resize(aShape.size() + 1);
    for (size_t i = 0; i < aShape.size(); ++i)
    {
        itsAdiosShape[i + 1] = aShape[aShape.size() - i - 1];
        itsAdiosCount[i + 1] = aShape[aShape.size() - i - 1];
        itsAdiosStart[i + 1] = 0;
    }
}

IPosition Adios2StManColumn::shape(uInt aRowNr)
{
    if(isShapeFixed)
    {
        return itsCasaShape;
    }
    else
    {
        auto shape = itsCasaShapes.find(aRowNr);
        if(shape != itsCasaShapes.end())
        {
            return shape->second;
        }
        else
        {
            /*
            throw(std::runtime_error("Shape not defined for Column "
                        + static_cast<std::string>(itsColumnName)
                        + " Row " + std::to_string(aRowNr)));
                        */
            return IPosition();
        }
    }
}

Bool Adios2StManColumn::canChangeShape() const
{
    return !isShapeFixed;
}

void Adios2StManColumn::setShape (uInt aRowNr, const IPosition& aShape)
{
    itsCasaShapes[aRowNr] = aShape;
}

void Adios2StManColumn::scalarVToSelection(uInt rownr)
{
    itsAdiosStart[0] = rownr;
    itsAdiosCount[0] = 1;
}

void Adios2StManColumn::arrayVToSelection(uInt rownr)
{
    itsAdiosStart[0] = rownr;
    itsAdiosCount[0] = 1;
    for (size_t i = 1; i < itsAdiosShape.size(); ++i)
    {
        itsAdiosStart[i] = 0;
        itsAdiosCount[i] = itsAdiosShape[i];
    }
}

void Adios2StManColumn::scalarColumnCellsVToSelection(const RefRows &rows)
{
    RefRowsSliceIter iter(rows);
    auto row_start = iter.sliceStart();
    auto row_end = iter.sliceEnd();
    iter.next();
    if (!iter.pastEnd()) {
        throw std::runtime_error("Adios2StManColumn::scalarColumnCellsVToSelection supports single slices");
    }
    itsAdiosStart[0] = row_start;
    itsAdiosCount[0] = row_end - row_start + 1;
    for (size_t i = 1; i < itsAdiosShape.size(); ++i)
    {
        itsAdiosStart[i] = 0;
        itsAdiosCount[i] = itsAdiosShape[i];
    }
}

void Adios2StManColumn::sliceVToSelection(uInt rownr, const Slicer &ns)
{
    columnSliceCellsVToSelection(rownr, 1, ns);
}

void Adios2StManColumn::columnSliceVToSelection(const Slicer &ns)
{
    columnSliceCellsVToSelection(0, itsStManPtr->getNrRows(), ns);
}

void Adios2StManColumn::columnSliceCellsVToSelection(const RefRows &rows, const Slicer &ns)
{
    RefRowsSliceIter iter(rows);
    auto row_start = iter.sliceStart();
    auto row_end = iter.sliceEnd();
    iter.next();
    if (!iter.pastEnd()) {
        throw std::runtime_error("Adios2StManColumn::columnSliceCellsVToSelection supports single slices");
    }
    columnSliceCellsVToSelection(row_start, row_end - row_start + 1, ns);
}

void Adios2StManColumn::columnSliceCellsVToSelection(uInt row_start, uInt row_count, const Slicer &ns)
{
    itsAdiosStart[0] = row_start;
    itsAdiosCount[0] = row_count;
    for (size_t i = 1; i < itsAdiosShape.size(); ++i)
    {
        itsAdiosStart[i] = ns.start()(ns.ndim() - i);
        itsAdiosCount[i] = ns.length()(ns.ndim() - i);
    }
}

#define DEFINE_GETPUT_TYPE_V(T) \
void Adios2StManColumn::put ## T ## V(uInt rownr, const T *dataPtr) \
{ \
    putScalarV(rownr, dataPtr); \
} \
\
void Adios2StManColumn::get ## T ## V(uInt rownr, T *dataPtr) \
{ \
    getScalarV(rownr, dataPtr); \
}

#define DEFINE_GETPUT_SCALAR_COLUMN_CELLS_V(T) \
void Adios2StManColumn::getScalarColumnCells ## T ## V(const RefRows& rownrs, Vector<T>* dataPtr) \
{ \
    getScalarColumnCellsV(rownrs, dataPtr); \
} \
\
void Adios2StManColumn::putScalarColumnCells ## T ## V(const RefRows& rownrs, const Vector<T>* dataPtr) \
{ \
    putScalarColumnCellsV(rownrs, dataPtr); \
}

#define DEFINE_GETPUT_SLICE_TYPE_V(T) \
void Adios2StManColumn::putSlice ## T ## V(uInt rownr, const Slicer& ns, const Array<T>* dataPtr) \
{ \
    putSliceV(rownr, ns, dataPtr); \
}\
\
void Adios2StManColumn::getSlice ## T ## V(uInt rownr, const Slicer& ns, Array<T>* dataPtr) \
{ \
    getSliceV(rownr, ns, dataPtr); \
}

#define DEFINE_GETPUT(T) \
DEFINE_GETPUT_TYPE_V(T) \
DEFINE_GETPUT_SCALAR_COLUMN_CELLS_V(T) \
DEFINE_GETPUT_SLICE_TYPE_V(T)

DEFINE_GETPUT(Bool)
DEFINE_GETPUT(uChar)
DEFINE_GETPUT(Short)
DEFINE_GETPUT(uShort)
DEFINE_GETPUT(Int)
DEFINE_GETPUT(uInt)
DEFINE_GETPUT(float)
DEFINE_GETPUT(double)
DEFINE_GETPUT(Complex)
DEFINE_GETPUT(DComplex)
DEFINE_GETPUT_TYPE_V(Int64)
DEFINE_GETPUT_SLICE_TYPE_V(String)
DEFINE_GETPUT_SCALAR_COLUMN_CELLS_V(Int64)
#undef DEFINE_GETPUT
#undef DEFINE_GETPUT_TYPE_V
#undef DEFINE_GETPUT_SCALAR_COLUMN_CELLS_V
#undef DEFINE_GETPUT_SLICE_TYPE_V


// string

template<>
void Adios2StManColumnT<std::string>::create(std::shared_ptr<adios2::Engine> aAdiosEngine, char /*aOpenMode*/)
{
    itsAdiosEngine = aAdiosEngine;
}

void Adios2StManColumn::putStringV(uInt rownr, const String *dataPtr)
{
    std::string variableName = static_cast<std::string>(itsColumnName) + std::to_string(rownr);
    adios2::Variable<std::string> v = itsAdiosIO->InquireVariable<std::string>(variableName);
    if (!v)
    {
        v = itsAdiosIO->DefineVariable<std::string>(variableName);
    }
    itsAdiosEngine->Put(v, reinterpret_cast<const std::string *>(dataPtr), adios2::Mode::Sync);
}

void Adios2StManColumn::getStringV(uInt rownr, String *dataPtr)
{
    std::string variableName = static_cast<std::string>(itsColumnName) + std::to_string(rownr);
    adios2::Variable<std::string> v = itsAdiosIO->InquireVariable<std::string>(variableName);
    if (v)
    {
        itsAdiosEngine->Get(v, reinterpret_cast<std::string *>(dataPtr), adios2::Mode::Sync);
    }
}

template<>
void Adios2StManColumnT<std::string>::putArrayV(uInt rownr, const void *dataPtr)
{
    String combined;
    Bool deleteIt;
    const String *data = (reinterpret_cast<const Array<String> *>(dataPtr))->getStorage(deleteIt);
    for(auto &i : *(reinterpret_cast<const Array<String> *>(dataPtr)))
    {
        combined = combined + i + itsStringArrayBarrier;
    }
    (reinterpret_cast<const Array<String> *>(dataPtr))->freeStorage(reinterpret_cast<const String *&>(data), deleteIt);
    putStringV(rownr, &combined);
}

template<>
void Adios2StManColumnT<std::string>::getArrayV(uInt rownr, void *dataPtr)
{
    String combined;
    getStringV(rownr, &combined);
    Bool deleteIt;
    String *data = (reinterpret_cast<Array<String>*>(dataPtr))->getStorage(deleteIt);
    size_t pos = 0;
    for(auto &i : *(reinterpret_cast<Array<String> *>(dataPtr)))
    {
        size_t found = combined.find(itsStringArrayBarrier, pos);
        if(found != std::string::npos)
        {
            i = combined.substr(pos, found - pos);
            pos = found + itsStringArrayBarrier.length();
        }
    }
    reinterpret_cast<Array<String>*>(dataPtr)->putStorage(reinterpret_cast<String *&>(data), deleteIt);
}

template<>
void Adios2StManColumnT<std::string>::getSliceV(uInt /*aRowNr*/, const Slicer &/*ns*/, void */*dataPtr*/)
{
    throw std::runtime_error("Not implemented yet");
}

template<>
void Adios2StManColumnT<std::string>::putSliceV(uInt /*aRowNr*/, const Slicer &/*ns*/, const void */*dataPtr*/)
{
    throw std::runtime_error("Not implemented yet");
}

template<>
void Adios2StManColumnT<std::string>::getColumnSliceV(const Slicer &/*ns*/, void */*dataPtr*/)
{
    throw std::runtime_error("Not implemented yet");
}

template<>
void Adios2StManColumnT<std::string>::putColumnSliceV(const Slicer &/*ns*/, const void */*dataPtr*/)
{
    throw std::runtime_error("Not implemented yet");
}

template<>
void Adios2StManColumnT<std::string>::getColumnSliceCellsV(const RefRows& /*rownrs*/, const Slicer& /*slicer*/, void* /*dataPtr*/)
{
    throw std::runtime_error("Not implemented yet");
}

template<>
void Adios2StManColumnT<std::string>::putColumnSliceCellsV(const RefRows& /*rownrs*/, const Slicer& /*slicer*/, const void* /*dataPtr*/)
{
    throw std::runtime_error("Not implemented yet");
}

} // namespace casacore
