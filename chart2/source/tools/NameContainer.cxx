/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * This file incorporates work covered by the following license notice:
 *
 *   Licensed to the Apache Software Foundation (ASF) under one or more
 *   contributor license agreements. See the NOTICE file distributed
 *   with this work for additional information regarding copyright
 *   ownership. The ASF licenses this file to you under the Apache
 *   License, Version 2.0 (the "License"); you may not use this file
 *   except in compliance with the License. You may obtain a copy of
 *   the License at http://www.apache.org/licenses/LICENSE-2.0 .
 */

#include <NameContainer.hxx>

#include <com/sun/star/uno/Any.hxx>
 #include <cppuhelper/supportsservice.hxx>

using namespace ::com::sun::star;
using ::com::sun::star::uno::Sequence;
using ::com::sun::star::uno::Any;

namespace chart
{

uno::Reference< container::XNameContainer > createNameContainer(
        const css::uno::Type& rType, const OUString& rServicename, const OUString& rImplementationName )
{
    return new NameContainer( rType, rServicename, rImplementationName );
}

NameContainer::NameContainer( const css::uno::Type& rType, const OUString& rServicename, const OUString& rImplementationName )
    : m_aType( rType )
    , m_aServicename( rServicename )
    , m_aImplementationName( rImplementationName )
{
}

NameContainer::NameContainer(
    const NameContainer & rOther )
    : impl::NameContainer_Base(rOther)
    , m_aType( rOther.m_aType )
    , m_aServicename( rOther.m_aServicename )
    , m_aImplementationName( rOther.m_aImplementationName )
    , m_aMap( rOther.m_aMap )
{
}

NameContainer::~NameContainer()
{
}

//XServiceInfo
OUString SAL_CALL NameContainer::getImplementationName()
{
    return m_aImplementationName;
}

sal_Bool SAL_CALL NameContainer::supportsService( const OUString& ServiceName )
{
    return cppu::supportsService(this, ServiceName);
}

Sequence< OUString > SAL_CALL NameContainer::getSupportedServiceNames()
{
    return { m_aServicename };
}

// XNameContainer
void SAL_CALL NameContainer::insertByName( const OUString& rName, const Any& rElement )
{
    if( m_aMap.find( rName ) != m_aMap.end() )
        throw container::ElementExistException();
    m_aMap.emplace( rName, rElement );
}

void SAL_CALL NameContainer::removeByName( const OUString& Name )
{
    tContentMap::iterator aIt( m_aMap.find( Name ));
    if( aIt == m_aMap.end())
        throw container::NoSuchElementException();
    m_aMap.erase( aIt );
}

// XNameReplace
void SAL_CALL NameContainer::replaceByName( const OUString& rName, const Any& rElement )
{
    tContentMap::iterator aIt( m_aMap.find( rName ));
    if( aIt == m_aMap.end() )
        throw container::NoSuchElementException();
    aIt->second = rElement;
}

// XNameAccess
Any SAL_CALL NameContainer::getByName( const OUString& rName )
{
    tContentMap::iterator aIter( m_aMap.find( rName ) );
    if( aIter == m_aMap.end() )
        throw container::NoSuchElementException();
    return aIter->second;
}

Sequence< OUString > SAL_CALL NameContainer::getElementNames()
{
    sal_Int32 nCount = m_aMap.size();
    Sequence< OUString > aSeq(nCount);
    sal_Int32 nN = 0;
    for (auto const& elem : m_aMap)
    {
        aSeq[nN++]=elem.first;
    }
    return aSeq;
}

sal_Bool SAL_CALL NameContainer::hasByName( const OUString& rName )
{
    return ( m_aMap.find( rName ) != m_aMap.end() );
}

// XElementAccess
sal_Bool SAL_CALL NameContainer::hasElements()
{
    return ! m_aMap.empty();
}

uno::Type SAL_CALL NameContainer::getElementType()
{
    return m_aType;
}

// XCloneable
uno::Reference< util::XCloneable > SAL_CALL NameContainer::createClone()
{
    return uno::Reference< util::XCloneable >( new NameContainer( *this ));
}

} //namespace chart

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
