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


/**************************************************************************
                                TODO
 **************************************************************************

 *************************************************************************/
#include <com/sun/star/beans/PropertyAttribute.hpp>

#include "ftpdynresultset.hxx"
#include "ftpresultsetfactory.hxx"
#include "ftpresultsetI.hxx"
#include "ftpcontent.hxx"
#include "ftpcontentprovider.hxx"
#include "ftpdirp.hxx"
#include "ftpcontentidentifier.hxx"
#include "ftpintreq.hxx"

#include <memory>
#include <vector>
#include <string.h>
#include "curl.hxx"
#include <comphelper/propertysequence.hxx>
#include <cppuhelper/queryinterface.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <cppuhelper/typeprovider.hxx>
#include <ucbhelper/cancelcommandexecution.hxx>
#include <ucbhelper/fd_inputstream.hxx>
#include <ucbhelper/propertyvalueset.hxx>
#include <ucbhelper/simpleauthenticationrequest.hxx>
#include <com/sun/star/lang/IllegalAccessException.hpp>
#include <com/sun/star/lang/NoSupportException.hpp>
#include <com/sun/star/ucb/ContentInfoAttribute.hpp>
#include <com/sun/star/ucb/UnsupportedCommandException.hpp>
#include <com/sun/star/beans/IllegalTypeException.hpp>
#include <com/sun/star/beans/UnknownPropertyException.hpp>
#include <com/sun/star/beans/Property.hpp>
#include <com/sun/star/ucb/XCommandInfo.hpp>
#include <com/sun/star/io/BufferSizeExceededException.hpp>
#include <com/sun/star/io/IOException.hpp>
#include <com/sun/star/io/NotConnectedException.hpp>
#include <com/sun/star/io/XActiveDataSink.hpp>
#include <com/sun/star/io/XOutputStream.hpp>
#include <com/sun/star/ucb/UnsupportedDataSinkException.hpp>
#include <com/sun/star/ucb/OpenCommandArgument2.hpp>
#include <com/sun/star/ucb/UnsupportedOpenModeException.hpp>
#include <com/sun/star/ucb/InteractiveNetworkConnectException.hpp>
#include <com/sun/star/ucb/InteractiveNetworkResolveNameException.hpp>
#include <com/sun/star/ucb/InteractiveIOException.hpp>
#include <com/sun/star/ucb/MissingPropertiesException.hpp>
#include <com/sun/star/ucb/MissingInputStreamException.hpp>
#include <com/sun/star/ucb/UnsupportedNameClashException.hpp>
#include <com/sun/star/ucb/OpenMode.hpp>
#include <com/sun/star/ucb/IOErrorCode.hpp>

using namespace ftp;
using namespace com::sun::star::task;
using namespace com::sun::star::container;
using namespace com::sun::star::lang;
using namespace com::sun::star::uno;
using namespace com::sun::star::ucb;
using namespace com::sun::star::beans;
using namespace com::sun::star::io;
using namespace com::sun::star::sdbc;


// Content Implementation.

FTPContent::FTPContent( const Reference< XComponentContext >& rxContext,
                        FTPContentProvider* pProvider,
                        const Reference< XContentIdentifier >& Identifier,
                        const FTPURL& aFTPURL)
    : ContentImplHelper(rxContext,pProvider,Identifier)
    , m_pFCP(pProvider)
    , m_aFTPURL(aFTPURL)
    , m_bInserted(false)
    , m_bTitleSet(false)
{
}

FTPContent::FTPContent( const Reference< XComponentContext >& rxContext,
                        FTPContentProvider* pProvider,
                        const Reference< XContentIdentifier >& Identifier,
                        const ContentInfo& Info)
    : ContentImplHelper(rxContext,pProvider,Identifier)
    , m_pFCP(pProvider)
    , m_aFTPURL(Identifier->getContentIdentifier(), pProvider)
    , m_bInserted(true)
    , m_bTitleSet(false)
    , m_aInfo(Info)
{
}

FTPContent::~FTPContent()
{
}

// XInterface methods.

void SAL_CALL FTPContent::acquire()
    noexcept
{
    OWeakObject::acquire();
}

void SAL_CALL FTPContent::release()
    noexcept
{
    OWeakObject::release();
}

css::uno::Any SAL_CALL FTPContent::queryInterface( const css::uno::Type & rType )
{
    css::uno::Any aRet = cppu::queryInterface( rType,
                                               static_cast< XTypeProvider* >(this),
                                               static_cast< XServiceInfo* >(this),
                                               static_cast< XContent* >(this),
                                               static_cast< XCommandProcessor* >(this),
                                               static_cast< XContentCreator* >(this),
                                               static_cast< XChild* >(this)
                                               );
    return aRet.hasValue() ? aRet : OWeakObject::queryInterface( rType );
}

// XTypeProvider methods.

css::uno::Sequence< sal_Int8 > SAL_CALL FTPContent::getImplementationId()
{
    return css::uno::Sequence<sal_Int8>();
}

css::uno::Sequence< css::uno::Type > SAL_CALL FTPContent::getTypes()
{
    static cppu::OTypeCollection s_aCollection(
                    cppu::UnoType<XTypeProvider>::get(),
                    cppu::UnoType<XServiceInfo>::get(),
                    cppu::UnoType<XContent>::get(),
                    cppu::UnoType<XCommandProcessor>::get(),
                    cppu::UnoType<XContentCreator>::get(),
                    cppu::UnoType<XChild>::get()
                );

    return s_aCollection.getTypes();
}


// XServiceInfo methods.

OUString SAL_CALL FTPContent::getImplementationName()
{
    return "com.sun.star.comp.FTPContent";
}

sal_Bool SAL_CALL FTPContent::supportsService( const OUString& ServiceName )
{
    return cppu::supportsService( this, ServiceName );
}

css::uno::Sequence< OUString > SAL_CALL FTPContent::getSupportedServiceNames()
{
    return { "com.sun.star.ucb.FTPContent" };
}


// XContent methods.

// virtual
OUString SAL_CALL FTPContent::getContentType()
{
    return FTP_CONTENT_TYPE;
}

// XCommandProcessor methods.

//virtual
void SAL_CALL FTPContent::abort( sal_Int32 /*CommandId*/ )
{
}


ResultSetFactory::ResultSetFactory(const Reference<XComponentContext >&  rxContext,
                  const Reference<XContentProvider >&  xProvider,
                  const Sequence<Property>& seq,
                  std::vector<FTPDirentry>&& dirvec)
        : m_xContext(rxContext),
          m_xProvider(xProvider),
          m_seq(seq),
          m_dirvec(std::move(dirvec))
{
}


rtl::Reference<ResultSetBase> ResultSetFactory::createResultSet()
{
    return new ResultSetI(m_xContext,
                          m_xProvider,
                          m_seq,
                          m_dirvec);
}


// XCommandProcessor methods.

namespace {

enum ACTION { NOACTION,
              THROWAUTHENTICATIONREQUEST,
              THROWACCESSDENIED,
              THROWINTERACTIVECONNECT,
              THROWRESOLVENAME,
              THROWQUOTE,
              THROWNOFILE,
              THROWGENERAL };

}

// virtual
Any SAL_CALL FTPContent::execute( const Command& aCommand,
                                  sal_Int32 /*CommandId*/,
                                  const Reference<
                                  XCommandEnvironment >& Environment)
{
    ACTION action(NOACTION);
    Any aRet;

    while(true)
    {
        try
        {
            if(action == THROWAUTHENTICATIONREQUEST)
            {
                // try to get a continuation first
                OUString aPassword,aAccount;
                m_pFCP->forHost(m_aFTPURL.host(),
                                m_aFTPURL.port(),
                                m_aFTPURL.username(),
                                aPassword,
                                aAccount);
                rtl::Reference<ucbhelper::SimpleAuthenticationRequest>
                    p( new ucbhelper::SimpleAuthenticationRequest(
                        m_aFTPURL.ident(false, false),
                        m_aFTPURL.host(),      // ServerName
                        ucbhelper::SimpleAuthenticationRequest::ENTITY_NA,
                        OUString(),
                        ucbhelper::SimpleAuthenticationRequest
                        ::ENTITY_FIXED,
                        m_aFTPURL.username(),
                        ucbhelper::SimpleAuthenticationRequest
                        ::ENTITY_MODIFY,
                        aPassword));

                Reference<XInteractionHandler> xInteractionHandler;
                if(Environment.is())
                    xInteractionHandler =
                        Environment->getInteractionHandler();

                if( xInteractionHandler.is()) {
                    xInteractionHandler->handle(p);

                    Reference<XInterface> xSelection(
                        p->getSelection());

                    if(Reference<XInteractionRetry>(
                        xSelection,UNO_QUERY).is())
                        action = NOACTION;
                    else if(Reference<XInteractionSupplyAuthentication>(
                        xSelection,UNO_QUERY).is()) {
                        m_pFCP->setHost(
                            m_aFTPURL.host(),
                            m_aFTPURL.port(),
                            m_aFTPURL.username(),
                            p->getAuthenticationSupplier()->getPassword(),
                            aAccount);
                        action = NOACTION;
                    }
                }
                aRet = p->getRequest();
            }

//              if(aCommand.Name.equalsAscii(
//                  "getPropertyValues") &&
//                 action != NOACTION) {
//                  // It is not allowed to throw if
//                  // command is getPropertyValues
//                  rtl::Reference<ucbhelper::PropertyValueSet> xRow =
//                      new ucbhelper::PropertyValueSet(m_xSMgr);
//                  Sequence<Property> Properties;
//                  aCommand.Argument >>= Properties;
//                  for(int i = 0; i < Properties.getLength(); ++i)
//                      xRow->appendVoid(Properties[i]);
//                  aRet <<= Reference<XRow>(xRow.get());
//                  return aRet;
//              }

            switch (action)
            {
            case NOACTION:
                break;

            case THROWAUTHENTICATIONREQUEST:
                ucbhelper::cancelCommandExecution(
                    aRet,
                    Reference<XCommandEnvironment>(nullptr));
                break;

            case THROWACCESSDENIED:
                {
                    Sequence<Any> seq(comphelper::InitAnyPropertySequence(
                    {
                        {"Uri", Any(m_aFTPURL.ident(false,false))}
                    }));
                    ucbhelper::cancelCommandExecution(
                        IOErrorCode_ACCESS_DENIED,
                        seq,
                        Environment);
                    break;
                }
            case THROWINTERACTIVECONNECT:
                {
                    InteractiveNetworkConnectException excep;
                    excep.Server = m_aFTPURL.host();
                    aRet <<= excep;
                    ucbhelper::cancelCommandExecution(
                        aRet,
                        Environment);
                    break;
                }
            case THROWRESOLVENAME:
                {
                    InteractiveNetworkResolveNameException excep;
                    excep.Server = m_aFTPURL.host();
                    aRet <<= excep;
                    ucbhelper::cancelCommandExecution(
                        aRet,
                        Environment);
                    break;
                }
            case THROWNOFILE:
                {
                    Sequence<Any> seq(comphelper::InitAnyPropertySequence(
                    {
                        {"Uri", Any(m_aFTPURL.ident(false,false))}
                    }));
                    ucbhelper::cancelCommandExecution(
                        IOErrorCode_NO_FILE,
                        seq,
                        Environment);
                    break;
                }
            case THROWQUOTE:
            case THROWGENERAL:
                ucbhelper::cancelCommandExecution(
                    IOErrorCode_GENERAL,
                    Sequence<Any>(0),
                    Environment);
                break;
            }

            if(aCommand.Name == "getPropertyValues") {
                Sequence<Property> Properties;
                if(!(aCommand.Argument >>= Properties))
                {
                    aRet <<= IllegalArgumentException(
                                "Wrong argument type!",
                                static_cast< cppu::OWeakObject * >(this),
                                -1);
                    ucbhelper::cancelCommandExecution(aRet,Environment);
                }

                aRet <<= getPropertyValues(Properties);
            }
            else if(aCommand.Name == "setPropertyValues")
            {
                Sequence<PropertyValue> propertyValues;

                if( ! ( aCommand.Argument >>= propertyValues ) ) {
                    aRet <<= IllegalArgumentException(
                                "Wrong argument type!",
                                static_cast< cppu::OWeakObject * >(this),
                                -1);
                    ucbhelper::cancelCommandExecution(aRet,Environment);
                }

                aRet <<= setPropertyValues(propertyValues);
            }
            else if(aCommand.Name == "getCommandInfo") {
                // Note: Implemented by base class.
                aRet <<= getCommandInfo(Environment);
            }
            else if(aCommand.Name == "getPropertySetInfo") {
                // Note: Implemented by base class.
                aRet <<= getPropertySetInfo(Environment);
            }
            else if(aCommand.Name == "insert")
            {
                InsertCommandArgument aInsertArgument;
                if ( ! ( aCommand.Argument >>= aInsertArgument ) ) {
                    aRet <<= IllegalArgumentException(
                                "Wrong argument type!",
                                static_cast< cppu::OWeakObject * >(this),
                                -1);
                    ucbhelper::cancelCommandExecution(aRet,Environment);
                }
                insert(aInsertArgument,Environment);
            }
            else if(aCommand.Name == "delete") {
                m_aFTPURL.del();
                deleted();
            }
            else if(aCommand.Name == "open") {
                OpenCommandArgument2 aOpenCommand;
                if ( !( aCommand.Argument >>= aOpenCommand ) ) {
                    aRet <<= IllegalArgumentException(
                                "Wrong argument type!",
                                static_cast< cppu::OWeakObject * >(this),
                                -1);

                    ucbhelper::cancelCommandExecution(aRet,Environment);
                }

                if(aOpenCommand.Mode == OpenMode::DOCUMENT) {
                    // Open as a document
                    Reference<XActiveDataSink>
                        xActiveDataSink(aOpenCommand.Sink,UNO_QUERY);
                    Reference< XOutputStream >
                        xOutputStream(aOpenCommand.Sink,UNO_QUERY);

                    if(xActiveDataSink.is()) {
                        xActiveDataSink->setInputStream(
                            new ucbhelper::FdInputStream(m_aFTPURL.open()));
                    }
                    else if(xOutputStream.is()) {
                        Reference<XInputStream> xStream(
                            new ucbhelper::FdInputStream(m_aFTPURL.open()));
                        for (;;) {
                            Sequence<sal_Int8> byte_seq(4096);
                            sal_Int32 n = xStream->readBytes(byte_seq, 4096);
                            if (n == 0) {
                                break;
                            }
                            try {
                                if(byte_seq.getLength() != n)
                                    byte_seq.realloc(n);
                                xOutputStream->writeBytes(byte_seq);
                            } catch(const NotConnectedException&) {

                            } catch(const BufferSizeExceededException&) {

                            } catch(const IOException&) {

                            }
                        }
                    }
                    else {
                        aRet <<= UnsupportedDataSinkException(
                            OUString(),
                            static_cast< cppu::OWeakObject * >(this),
                            aOpenCommand.Sink);
                        ucbhelper::cancelCommandExecution(aRet,Environment);
                    }
                }
                else if(aOpenCommand.Mode == OpenMode::ALL ||
                        aOpenCommand.Mode == OpenMode::DOCUMENTS ||
                        aOpenCommand.Mode == OpenMode::FOLDERS ) {
                    std::vector<FTPDirentry> resvec =
                        m_aFTPURL.list(sal_Int16(aOpenCommand.Mode));
                    Reference< XDynamicResultSet > xSet
                        = new DynamicResultSet(
                            m_xContext,
                            aOpenCommand,
                            std::make_unique<ResultSetFactory>(m_xContext,
                                                 m_xProvider.get(),
                                                 aOpenCommand.Properties,
                                                 std::move(resvec)));
                    aRet <<= xSet;
                }
                else if(aOpenCommand.Mode ==
                        OpenMode::DOCUMENT_SHARE_DENY_NONE ||
                        aOpenCommand.Mode ==
                        OpenMode::DOCUMENT_SHARE_DENY_WRITE) {
                    // Unsupported OpenMode
                    aRet <<= UnsupportedOpenModeException(
                        OUString(),
                        static_cast< cppu::OWeakObject * >(this),
                        static_cast< sal_Int16 >(aOpenCommand.Mode));
                    ucbhelper::cancelCommandExecution(aRet,Environment);
                }
                else {
                    aRet <<= IllegalArgumentException(
                                "Unexpected OpenMode!",
                                static_cast< cppu::OWeakObject * >(this),
                                -1);

                    ucbhelper::cancelCommandExecution(aRet,Environment);
                }
            } else if(aCommand.Name == "createNewContent") {
                ContentInfo aArg;
                if (!(aCommand.Argument >>= aArg)) {
                    ucbhelper::cancelCommandExecution(
                        makeAny(
                            IllegalArgumentException(
                                "Wrong argument type!",
                                static_cast< cppu::OWeakObject * >(this),
                                -1)),
                        Environment);
                    // Unreachable
                }
                aRet <<= createNewContent(aArg);
            } else {
                aRet <<= UnsupportedCommandException(
                    aCommand.Name,
                    static_cast< cppu::OWeakObject * >(this));
                ucbhelper::cancelCommandExecution(aRet,Environment);
            }

            return aRet;
        }
        catch(const curl_exception& e)
        {
            if(e.code() == CURLE_COULDNT_CONNECT)
                action = THROWINTERACTIVECONNECT;
            else if(e.code() == CURLE_COULDNT_RESOLVE_HOST )
                action = THROWRESOLVENAME;
            else if(e.code() == CURLE_FTP_USER_PASSWORD_INCORRECT ||
                    e.code() == CURLE_LOGIN_DENIED ||
                    e.code() == CURLE_BAD_PASSWORD_ENTERED ||
                    e.code() == CURLE_FTP_WEIRD_PASS_REPLY)
                action = THROWAUTHENTICATIONREQUEST;
            else if(e.code() == CURLE_FTP_ACCESS_DENIED)
                action = THROWACCESSDENIED;
            else if(e.code() == CURLE_FTP_QUOTE_ERROR)
                action = THROWQUOTE;
            else if(e.code() == CURLE_FTP_COULDNT_RETR_FILE)
                action = THROWNOFILE;
            else
                // nothing known about the cause of the error
                action = THROWGENERAL;
        }
    }
}

#define FTP_FILE "application/vnd.sun.staroffice.ftp-file"

#define FTP_FOLDER "application/vnd.sun.staroffice.ftp-folder"

Sequence<ContentInfo > SAL_CALL
FTPContent::queryCreatableContentsInfo(  )
{
    return queryCreatableContentsInfo_Static();
}

// static
Sequence<ContentInfo >
FTPContent::queryCreatableContentsInfo_Static(  )
{
    Sequence< Property > props( 1 );
    props[0] = Property(
        "Title",
        -1,
        cppu::UnoType<OUString>::get(),
        PropertyAttribute::MAYBEVOID
        | PropertyAttribute::BOUND );
    return
    {
        { FTP_FILE, ContentInfoAttribute::INSERT_WITH_INPUTSTREAM | ContentInfoAttribute::KIND_DOCUMENT, props },
        { FTP_FOLDER, ContentInfoAttribute::KIND_FOLDER, props }
    };
}

Reference<XContent > SAL_CALL
FTPContent::createNewContent( const ContentInfo& Info )
{
    if( Info.Type =="application/vnd.sun.staroffice.ftp-file" || Info.Type == "application/vnd.sun.staroffice.ftp-folder" )
        return new FTPContent(m_xContext,
                              m_pFCP,
                              m_xIdentifier,Info);
    else
        return Reference<XContent>(nullptr);
}


Reference<XInterface > SAL_CALL
FTPContent::getParent(  )
{
    Reference<XContentIdentifier>
        xIdent(new FTPContentIdentifier(m_aFTPURL.parent()));
    return Reference<XInterface>( m_xProvider->queryContent(xIdent), UNO_QUERY );
}


void SAL_CALL
FTPContent::setParent(const Reference<XInterface >& /*Parent*/ )
{
    throw NoSupportException();
}


OUString FTPContent::getParentURL()
{
    return m_aFTPURL.parent();
}

namespace {

class InsertData
    : public CurlInput {

public:

    explicit InsertData(const Reference<XInputStream>& xInputStream)
        : m_xInputStream(xInputStream) { }
    virtual ~InsertData() {}

    // returns the number of bytes actually read
    virtual sal_Int32 read(sal_Int8 *dest,sal_Int32 nBytesRequested) override;

private:

    Reference<XInputStream> m_xInputStream;
};

}

sal_Int32 InsertData::read(sal_Int8 *dest,sal_Int32 nBytesRequested)
{
    sal_Int32 m = 0;

    if(m_xInputStream.is()) {
        Sequence<sal_Int8> seq(nBytesRequested);
        m = m_xInputStream->readBytes(seq,nBytesRequested);
        memcpy(dest,seq.getConstArray(),m);
    }
    return m;
}


void FTPContent::insert(const InsertCommandArgument& aInsertCommand,
                        const Reference<XCommandEnvironment>& Env)
{
    osl::MutexGuard aGuard(m_aMutex);

    if(m_bInserted && !m_bTitleSet) {
        MissingPropertiesException excep;
        excep.Properties.realloc(1);
        excep.Properties[0] = "Title";
        ucbhelper::cancelCommandExecution(Any(excep), Env);
    }

    if(m_bInserted &&
       m_aInfo.Type == FTP_FILE &&
       !aInsertCommand.Data.is())
    {
        MissingInputStreamException excep;
        ucbhelper::cancelCommandExecution(Any(excep), Env);
    }

    bool bReplace(aInsertCommand.ReplaceExisting);

 retry:
    try {
        if(m_aInfo.Type == FTP_FILE) {
            InsertData data(aInsertCommand.Data);
            m_aFTPURL.insert(bReplace,&data);
        } else if(m_aInfo.Type == FTP_FOLDER)
            m_aFTPURL.mkdir(bReplace);
    } catch(const curl_exception& e) {
        if(e.code() == FOLDER_MIGHT_EXIST_DURING_INSERT ||
                  e.code() == FILE_MIGHT_EXIST_DURING_INSERT) {
            // Interact
            Reference<XInteractionHandler> xInt;
            if(Env.is())
                xInt = Env->getInteractionHandler();

            UnsupportedNameClashException excep;
            excep.NameClash = 0; //NameClash::ERROR;

            if(!xInt.is()) {
                ucbhelper::cancelCommandExecution(Any(excep), Env);
            }

            XInteractionRequestImpl request;
            const Reference<XInteractionRequest>& xReq(request.getRequest());
            xInt->handle(xReq);
            if (request.approved()) {
                bReplace = true;
                goto retry;
            }
            else
                throw excep;
        }
        else
            throw;
    }

    // May not be reached, because both mkdir and insert can throw curl-
    // exceptions
    m_bInserted = false;
    inserted();
}


Reference< XRow > FTPContent::getPropertyValues(
    const Sequence< Property >& seqProp
)
{
    rtl::Reference<ucbhelper::PropertyValueSet> xRow =
        new ucbhelper::PropertyValueSet(m_xContext);

    FTPDirentry aDirEntry = m_aFTPURL.direntry();

    for(const auto& rProp : seqProp) {
        const OUString& Name = rProp.Name;
        if(Name == "Title")
            xRow->appendString(rProp,aDirEntry.m_aName);
        else if(Name == "CreatableContentsInfo")
            xRow->appendObject(rProp,
                               makeAny(queryCreatableContentsInfo()));
        else if(aDirEntry.m_nMode != INETCOREFTP_FILEMODE_UNKNOWN) {
            if(Name == "ContentType")
                xRow->appendString(rProp,
                                   (aDirEntry.m_nMode & INETCOREFTP_FILEMODE_ISDIR)
                                   ? OUString(FTP_FOLDER)
                                   : OUString(FTP_FILE) );
            else if(Name == "IsReadOnly")
                xRow->appendBoolean(rProp,
                                    (aDirEntry.m_nMode
                                     & INETCOREFTP_FILEMODE_WRITE) == 0 );
            else if(Name == "IsDocument")
                xRow->appendBoolean(rProp,
                                    (aDirEntry.m_nMode &
                                               INETCOREFTP_FILEMODE_ISDIR) != INETCOREFTP_FILEMODE_ISDIR);
            else if(Name == "IsFolder")
                xRow->appendBoolean(rProp,
                                    (aDirEntry.m_nMode &
                                             INETCOREFTP_FILEMODE_ISDIR) == INETCOREFTP_FILEMODE_ISDIR);
            else if(Name == "Size")
                xRow->appendLong(rProp,
                                 aDirEntry.m_nSize);
            else if(Name == "DateCreated")
                xRow->appendTimestamp(rProp,
                                      aDirEntry.m_aDate);
            else
                xRow->appendVoid(rProp);
        } else
            xRow->appendVoid(rProp);
    }

    return xRow;
}


Sequence<Any> FTPContent::setPropertyValues(
    const Sequence<PropertyValue>& seqPropVal)
{
    Sequence<Any> ret(seqPropVal.getLength());
    Sequence<PropertyChangeEvent > evt;

    osl::MutexGuard aGuard(m_aMutex);
    for(sal_Int32 i = 0; i < ret.getLength(); ++i) {
        if ( seqPropVal[i].Name == "Title" ) {
            OUString Title;
            if(!(seqPropVal[i].Value >>= Title)) {
                ret[i] <<= IllegalTypeException();
                continue;
            } else if(Title.isEmpty()) {
                ret[i] <<= IllegalArgumentException();
                continue;
            }

            if(m_bInserted) {
                m_aFTPURL.child(Title);
                m_xIdentifier =
                    new FTPContentIdentifier(m_aFTPURL.ident(false,false));
                m_bTitleSet = true;
            } else
                try {
                    OUString OldTitle = m_aFTPURL.ren(Title);
                    evt.realloc(1);
                    evt[0].PropertyName = "Title";
                    evt[0].Further = false;
                    evt[0].PropertyHandle = -1;
                    evt[0].OldValue <<= OldTitle;
                    evt[0].NewValue <<= Title;
                } catch(const curl_exception&) {
                    InteractiveIOException excep;
                    // any better possibility here?
                    // ( the error code is always CURLE_FTP_QUOTE_ERROR )
                    excep.Code = IOErrorCode_ACCESS_DENIED;
                    ret[i] <<= excep;
                }
        } else {
            Sequence<Property> props =
                getProperties(Reference<XCommandEnvironment>(nullptr));

            // either unknown or read-only
            ret[i] <<= UnknownPropertyException();
            const auto& rName = seqPropVal[i].Name;
            auto pProp = std::find_if(props.begin(), props.end(),
                [&rName](const Property& rProp) { return rProp.Name == rName; });
            if (pProp != props.end()) {
                ret[i] <<= IllegalAccessException(
                    "Property is read-only!",
                        //props[j].Attributes & PropertyAttribute::READONLY
                        //    ? "Property is read-only!"
                        //    : "Access denied!"),
                    static_cast< cppu::OWeakObject * >( this ));
            }
        }
    }

    if(evt.hasElements()) {
        // title has changed
        notifyPropertiesChange(evt);
        (void)exchange(new FTPContentIdentifier(m_aFTPURL.ident(false,false)));
    }

    return ret;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
