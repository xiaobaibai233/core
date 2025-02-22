/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <sal/config.h>

#include <string_view>

#include <test/bootstrapfixture.hxx>
#include <unotest/macros_test.hxx>

#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/drawing/LineStyle.hpp>
#include <com/sun/star/drawing/PointSequence.hpp>
#include <com/sun/star/drawing/PointSequenceSequence.hpp>
#include <com/sun/star/drawing/PolygonKind.hpp>
#include <com/sun/star/drawing/PolyPolygonBezierCoords.hpp>
#include <com/sun/star/drawing/XDrawPagesSupplier.hpp>
#include <com/sun/star/frame/Desktop.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>

using namespace ::com::sun::star;

constexpr OUStringLiteral DATA_DIRECTORY = u"/oox/qa/unit/data/";

/// oox vml tests.
class OoxVmlTest : public test::BootstrapFixture, public unotest::MacrosTest
{
private:
    uno::Reference<lang::XComponent> mxComponent;

public:
    void setUp() override;
    void tearDown() override;
    uno::Reference<lang::XComponent>& getComponent() { return mxComponent; }
    void load(std::u16string_view rURL);
};

void OoxVmlTest::setUp()
{
    test::BootstrapFixture::setUp();

    mxDesktop.set(frame::Desktop::create(mxComponentContext));
}

void OoxVmlTest::tearDown()
{
    if (mxComponent.is())
        mxComponent->dispose();

    test::BootstrapFixture::tearDown();
}

void OoxVmlTest::load(std::u16string_view rFileName)
{
    OUString aURL = m_directories.getURLFromSrc(DATA_DIRECTORY) + rFileName;
    mxComponent = loadFromDesktop(aURL);
}

CPPUNIT_TEST_FIXTURE(OoxVmlTest, tdf112450_vml_polyline)
{
    // Load a document with v:polyline shapes. Error was, that the size was set to zero and the
    // points were zero because of missing decode from length with unit.
    load(u"tdf112450_vml_polyline.docx");
    uno::Reference<drawing::XDrawPagesSupplier> xDrawPagesSupplier(getComponent(), uno::UNO_QUERY);
    uno::Reference<drawing::XDrawPage> xDrawPage(xDrawPagesSupplier->getDrawPages()->getByIndex(0),
                                                 uno::UNO_QUERY);
    {
        // This tests a polyline shape which is not closed.
        uno::Reference<drawing::XShape> xShape(xDrawPage->getByIndex(0), uno::UNO_QUERY);
        uno::Reference<beans::XPropertySet> xShapeProps(xShape, uno::UNO_QUERY);
        // Without fix in place, Geometry had 2 points, both 0|0.
        drawing::PointSequenceSequence aGeometry;
        xShapeProps->getPropertyValue("Geometry") >>= aGeometry;
        drawing::PointSequence aPolygon = aGeometry[0];
        CPPUNIT_ASSERT_DOUBLES_EQUAL(sal_Int32(6879), aPolygon[3].X, 1);
        CPPUNIT_ASSERT_DOUBLES_EQUAL(sal_Int32(487), aPolygon[3].Y, 1);
        // Without fix in place, width and height were zero
        CPPUNIT_ASSERT_DOUBLES_EQUAL(sal_Int32(6879), xShape->getSize().Width, 1);
        CPPUNIT_ASSERT_DOUBLES_EQUAL(sal_Int32(1926), xShape->getSize().Height, 1);
        // After the fix the shape has still to be PolygonKind_PLIN
        drawing::PolygonKind ePolygonKind;
        xShapeProps->getPropertyValue("PolygonKind") >>= ePolygonKind;
        CPPUNIT_ASSERT_EQUAL(drawing::PolygonKind_PLIN, ePolygonKind);
    }
    {
        uno::Reference<drawing::XShape> xShape(xDrawPage->getByIndex(1), uno::UNO_QUERY);
        uno::Reference<beans::XPropertySet> xShapeProps(xShape, uno::UNO_QUERY);
        // Without fix in place, Geometry had 2 points, both 0|0.
        drawing::PointSequenceSequence aGeometry;
        xShapeProps->getPropertyValue("Geometry") >>= aGeometry;
        drawing::PointSequence aPolygon = aGeometry[0];
        CPPUNIT_ASSERT_DOUBLES_EQUAL(sal_Int32(5062), aPolygon[2].X, 1);
        CPPUNIT_ASSERT_DOUBLES_EQUAL(sal_Int32(2247), aPolygon[2].Y, 1);
        // Without fix in place, width and height were zero
        CPPUNIT_ASSERT_DOUBLES_EQUAL(sal_Int32(6163), xShape->getSize().Width, 1);
        CPPUNIT_ASSERT_DOUBLES_EQUAL(sal_Int32(2247), xShape->getSize().Height, 1);
        // Without fix in place the shape was not closed, it had PolygonKind_PLIN
        drawing::PolygonKind ePolygonKind;
        xShapeProps->getPropertyValue("PolygonKind") >>= ePolygonKind;
        CPPUNIT_ASSERT_EQUAL(drawing::PolygonKind_POLY, ePolygonKind);
    }
    {
        // This tests a filled shape where v:polyline does not have attribute coordsize
        uno::Reference<drawing::XShape> xShape(xDrawPage->getByIndex(2), uno::UNO_QUERY);
        uno::Reference<beans::XPropertySet> xShapeProps(xShape, uno::UNO_QUERY);
        // Without fix in place, Geometry had 2 points, both 0|0.
        drawing::PointSequenceSequence aGeometry;
        xShapeProps->getPropertyValue("Geometry") >>= aGeometry;
        drawing::PointSequence aPolygon = aGeometry[0];
        CPPUNIT_ASSERT_DOUBLES_EQUAL(sal_Int32(2095), aPolygon[3].X, 1);
        CPPUNIT_ASSERT_DOUBLES_EQUAL(sal_Int32(608), aPolygon[3].Y, 1);
        // Without fix in place, width and height were zero
        CPPUNIT_ASSERT_DOUBLES_EQUAL(sal_Int32(5634), xShape->getSize().Width, 1);
        CPPUNIT_ASSERT_DOUBLES_EQUAL(sal_Int32(2485), xShape->getSize().Height, 1);
        // Without fix in place the shape was not closed, it had PolygonKind_PLIN
        drawing::PolygonKind ePolygonKind;
        xShapeProps->getPropertyValue("PolygonKind") >>= ePolygonKind;
        CPPUNIT_ASSERT_EQUAL(drawing::PolygonKind_POLY, ePolygonKind);
    }
}

CPPUNIT_TEST_FIXTURE(OoxVmlTest, tdf137314_vml_rotation_unit_fd)
{
    // Load a document with a 30deg rotated arc on a drawing canvas. Rotation is given
    // as 1966080fd. Error was, that the vml angle unit "fd" was not converted to Degree100.
    load(u"tdf137314_vml_rotation_unit_fd.docx");
    uno::Reference<drawing::XDrawPagesSupplier> xDrawPagesSupplier(getComponent(), uno::UNO_QUERY);
    uno::Reference<drawing::XDrawPage> xDrawPage(xDrawPagesSupplier->getDrawPages()->getByIndex(0),
                                                 uno::UNO_QUERY);
    uno::Reference<container::XIndexAccess> xGroup(xDrawPage->getByIndex(0), uno::UNO_QUERY);
    uno::Reference<drawing::XShape> xShape(xGroup->getByIndex(1), uno::UNO_QUERY);
    uno::Reference<beans::XPropertySet> xShapeProps(xShape, uno::UNO_QUERY);
    drawing::PolyPolygonBezierCoords aPolyPolygonBezierCoords;
    xShapeProps->getPropertyValue("PolyPolygonBezier") >>= aPolyPolygonBezierCoords;
    drawing::PointSequence aPolygon = aPolyPolygonBezierCoords.Coordinates[1];
    // Without fix in place, the vector was -1441|1490.
    // [1] and [2] are Bezier-curve control points.
    sal_Int32 nDiffX = aPolygon[3].X - aPolygon[0].X;
    CPPUNIT_ASSERT_DOUBLES_EQUAL(sal_Int32(1490), nDiffX, 1);
    sal_Int32 nDiffY = aPolygon[3].Y - aPolygon[0].Y;
    CPPUNIT_ASSERT_DOUBLES_EQUAL(sal_Int32(1441), nDiffY, 1);
}

CPPUNIT_TEST_FIXTURE(OoxVmlTest, testSpt202ShapeType)
{
    // Load a document with a groupshape, 2nd child is a <v:shape>, its type has o:spt set to 202
    // (TextBox).
    load(u"group-spt202.docx");
    uno::Reference<drawing::XDrawPagesSupplier> xDrawPagesSupplier(getComponent(), uno::UNO_QUERY);
    uno::Reference<drawing::XDrawPage> xDrawPage(xDrawPagesSupplier->getDrawPages()->getByIndex(0),
                                                 uno::UNO_QUERY);
    uno::Reference<container::XIndexAccess> xGroup(xDrawPage->getByIndex(0), uno::UNO_QUERY);
    uno::Reference<drawing::XShape> xShape(xGroup->getByIndex(1), uno::UNO_QUERY);

    // Without the accompanying fix in place, this test would have failed with:
    // - Expected: com.sun.star.drawing.TextShape
    // - Actual  : com.sun.star.drawing.CustomShape
    // and then the size of the group shape was incorrect, e.g. its right edge was outside the page
    // boundaries.
    CPPUNIT_ASSERT_EQUAL(OUString("com.sun.star.drawing.TextShape"), xShape->getShapeType());
}

CPPUNIT_TEST_FIXTURE(OoxVmlTest, testShapeNonAutosizeWithText)
{
    // Load a document which has a group shape, containing a single child.
    // 17.78 cm is the full group shape width, 19431/64008 is the child shape's relative width inside
    // that, so 5.3975 cm should be the shape width.
    load(u"shape-non-autosize-with-text.docx");
    uno::Reference<drawing::XDrawPagesSupplier> xDrawPagesSupplier(getComponent(), uno::UNO_QUERY);
    uno::Reference<drawing::XDrawPage> xDrawPage(xDrawPagesSupplier->getDrawPages()->getByIndex(0),
                                                 uno::UNO_QUERY);
    uno::Reference<container::XIndexAccess> xGroup(xDrawPage->getByIndex(0), uno::UNO_QUERY);
    uno::Reference<drawing::XShape> xShape(xGroup->getByIndex(0), uno::UNO_QUERY);
    // Without the accompanying fix in place, this test would have failed with:
    // - Actual  : 1115
    // - Expected: 5398
    // because the width was determined using its text size, not using the explicit size.
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(5398), xShape->getSize().Width);
}

CPPUNIT_TEST_FIXTURE(OoxVmlTest, testGraphicStroke)
{
    load(u"graphic-stroke.pptx");
    uno::Reference<drawing::XDrawPagesSupplier> xDrawPagesSupplier(getComponent(), uno::UNO_QUERY);
    uno::Reference<drawing::XDrawPage> xDrawPage(xDrawPagesSupplier->getDrawPages()->getByIndex(0),
                                                 uno::UNO_QUERY);

    uno::Reference<beans::XPropertySet> xShape;
    for (sal_Int32 i = 0; i < xDrawPage->getCount(); ++i)
    {
        uno::Reference<lang::XServiceInfo> xInfo(xDrawPage->getByIndex(i), uno::UNO_QUERY);
        if (!xInfo->supportsService("com.sun.star.drawing.GraphicObjectShape"))
        {
            continue;
        }

        xShape.set(xInfo, uno::UNO_QUERY);
        break;
    }
    CPPUNIT_ASSERT(xShape.is());

    drawing::LineStyle eLineStyle{};
    xShape->getPropertyValue("LineStyle") >>= eLineStyle;
    // Without the accompanying fix in place, this test would have failed with:
    // - Expected: 1
    // - Actual  : 0
    // i.e. line style was NONE, not SOLID.
    CPPUNIT_ASSERT_EQUAL(drawing::LineStyle_SOLID, eLineStyle);
}

CPPUNIT_PLUGIN_IMPLEMENT();

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
