// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PDF_PDFIUM_PDFIUM_PAGE_H_
#define PDF_PDFIUM_PDFIUM_PAGE_H_

#include <map>
#include <set>
#include <string>
#include <vector>

#include "base/gtest_prod_util.h"
#include "base/optional.h"
#include "base/strings/string16.h"
#include "pdf/page_orientation.h"
#include "pdf/pdf_engine.h"
#include "ppapi/cpp/rect.h"
#include "third_party/pdfium/public/cpp/fpdf_scopers.h"
#include "third_party/pdfium/public/fpdf_doc.h"
#include "third_party/pdfium/public/fpdf_formfill.h"
#include "third_party/pdfium/public/fpdf_text.h"
#include "ui/gfx/geometry/point_f.h"

struct PP_PrivateAccessibilityTextRunInfo;

namespace chrome_pdf {

class PDFiumEngine;

// Wrapper around a page from the document.
class PDFiumPage {
 public:
  PDFiumPage(PDFiumEngine* engine, int i);
  PDFiumPage(PDFiumPage&& that);
  ~PDFiumPage();

  using IsValidLinkFunction = bool (*)(const std::string& url);
  static void SetIsValidLinkFunctionForTesting(IsValidLinkFunction function);

  // Unloads the PDFium data for this page from memory.
  void Unload();
  // Gets the FPDF_PAGE for this page, loading and parsing it if necessary.
  FPDF_PAGE GetPage();

  // Returns FPDF_TEXTPAGE for the page, loading and parsing it if necessary.
  FPDF_TEXTPAGE GetTextPage();

  // See definition of PDFEngine::GetTextRunInfo().
  base::Optional<PP_PrivateAccessibilityTextRunInfo> GetTextRunInfo(
      int start_char_index);
  // Get a unicode character from the page.
  uint32_t GetCharUnicode(int char_index);
  // Get the bounds of a character in page pixels.
  pp::FloatRect GetCharBounds(int char_index);

  enum Area {
    NONSELECTABLE_AREA,
    TEXT_AREA,       // Area contains regular, selectable text not
                     // within form fields.
    WEBLINK_AREA,    // Area is a hyperlink.
    DOCLINK_AREA,    // Area is a link to a different part of the same
                     // document.
    FORM_TEXT_AREA,  // Area is a form text field or form combobox text
                     // field.
  };

  struct LinkTarget {
    LinkTarget();
    LinkTarget(const LinkTarget& other);
    ~LinkTarget();

    // Valid for WEBLINK_AREA only.
    std::string url;

    // Valid for DOCLINK_AREA only.
    int page;
    // Valid for DOCLINK_AREA only. From the top of the page.
    base::Optional<float> y_in_pixels;
  };

  // Returns the (x, y) position of a destination in page coordinates.
  base::Optional<gfx::PointF> GetPageXYTarget(FPDF_DEST destination);

  // Transforms an (x, y) position in page coordinates to screen coordinates.
  gfx::PointF TransformPageToScreenXY(const gfx::PointF& xy);

  // Given a point in the document that's in this page, returns its character
  // index if it's near a character, and also the type of text.
  // Target is optional. It will be filled in for WEBLINK_AREA or
  // DOCLINK_AREA only.
  Area GetCharIndex(const pp::Point& point,
                    PageOrientation orientation,
                    int* char_index,
                    int* form_type,
                    LinkTarget* target);

  // Converts a form type to its corresponding Area.
  static Area FormTypeToArea(int form_type);

  // Gets the character at the given index.
  base::char16 GetCharAtIndex(int index);

  // Gets the number of characters in the page.
  int GetCharCount();

  // Given a rectangle in page coordinates, computes the range of continuous
  // characters which lie inside that rectangle. Returns false without
  // modifying the out parameters if no character lies inside the rectangle.
  bool GetUnderlyingTextRangeForRect(const pp::FloatRect& rect,
                                     int* start_index,
                                     int* char_len);

  // Converts from page coordinates to screen coordinates.
  pp::Rect PageToScreen(const pp::Point& offset,
                        double zoom,
                        double left,
                        double top,
                        double right,
                        double bottom,
                        PageOrientation orientation) const;

  const PDFEngine::PageFeatures* GetPageFeatures();

  int index() const { return index_; }

  const pp::Rect& rect() const { return rect_; }
  void set_rect(const pp::Rect& r) { rect_ = r; }

  // Availability is a one-way transition: A page can become available, but it
  // cannot become unavailable (unless deleted entirely).
  bool available() const { return available_; }
  void MarkAvailable() { available_ = true; }

  void set_calculated_links(bool calculated_links) {
    calculated_links_ = calculated_links;
  }

  FPDF_PAGE page() const { return page_.get(); }
  FPDF_TEXTPAGE text_page() const { return text_page_.get(); }

 private:
  friend class PDFiumPageLinkTest;
  friend class PDFiumTestBase;

  FRIEND_TEST_ALL_PREFIXES(PDFiumPageImageTest, TestCalculateImages);
  FRIEND_TEST_ALL_PREFIXES(PDFiumPageLinkTest, TestLinkGeneration);

  // Returns a link index if the given character index is over a link, or -1
  // otherwise.
  int GetLink(int char_index, LinkTarget* target);
  // Calculate the locations of any links on the page.
  void CalculateLinks();
  // Calculate the locations of images on the page.
  void CalculateImages();
  // Returns link type and fills target associated with a link. Returns
  // NONSELECTABLE_AREA if link detection failed.
  Area GetLinkTarget(FPDF_LINK link, LinkTarget* target);
  // Returns link type and fills target associated with a destination. Returns
  // NONSELECTABLE_AREA if detection failed.
  Area GetDestinationTarget(FPDF_DEST destination, LinkTarget* target);
  // Returns link type and fills target associated with a URI action. Returns
  // NONSELECTABLE_AREA if detection failed.
  Area GetURITarget(FPDF_ACTION uri_action, LinkTarget* target) const;
  // Calculates the set of character indices on which text runs need to be
  // broken for page objects such as links and images.
  void CalculatePageObjectTextRunBreaks();

  // Key    :  Marked content id for the image element as specified in the
  //           struct tree.
  // Value  :  Index of image in the |images_| vector.
  using MarkedContentIdToImageMap = std::map<int, size_t>;
  // Traverses the entire struct tree of the page recursively and extracts the
  // alt text from struct tree elements corresponding to the marked content IDs
  // present in |marked_content_id_image_map|.
  void PopulateImageAltText(
      const MarkedContentIdToImageMap& marked_content_id_image_map);
  // Traverses a struct element and its sub-tree recursively and extracts the
  // alt text from struct elements corresponding to the marked content IDs
  // present in |marked_content_id_image_map|. Uses |visited_elements| to guard
  // against malformed struct trees.
  void PopulateImageAltTextForStructElement(
      const MarkedContentIdToImageMap& marked_content_id_image_map,
      FPDF_STRUCTELEMENT current_element,
      std::set<FPDF_STRUCTELEMENT>* visited_elements);

  class ScopedUnloadPreventer {
   public:
    explicit ScopedUnloadPreventer(PDFiumPage* page);
    ~ScopedUnloadPreventer();

   private:
    PDFiumPage* const page_;
  };

  struct Link {
    Link();
    Link(const Link& that);
    ~Link();

    // Represents start index of underlying text range. Should be -1 if the link
    // is not over text.
    int32_t start_char_index = -1;
    // Represents the number of characters that the link overlaps with.
    int32_t char_count = 0;
    std::vector<pp::Rect> bounding_rects;

    // Valid for links with external urls only.
    std::string url;
  };

  // Represents an Image inside the page.
  struct Image {
    Image();
    Image(const Image& other);
    ~Image();

    pp::Rect bounding_rect;
    // Alt text is available only for tagged PDFs.
    std::string alt_text;
  };

  PDFiumEngine* engine_;
  ScopedFPDFPage page_;
  ScopedFPDFTextPage text_page_;
  int index_;
  int preventing_unload_count_ = 0;
  pp::Rect rect_;
  bool calculated_links_ = false;
  std::vector<Link> links_;
  bool calculated_images_ = false;
  std::vector<Image> images_;
  bool calculated_page_object_text_run_breaks_ = false;
  // The set of character indices on which text runs need to be broken for page
  // objects.
  std::set<int> page_object_text_run_breaks_;
  bool available_;
  PDFEngine::PageFeatures page_features_;

  DISALLOW_COPY_AND_ASSIGN(PDFiumPage);
};

// Converts page orientations to the PDFium equivalents, as defined by
// FPDF_RenderPage().
int ToPDFiumRotation(PageOrientation orientation);

}  // namespace chrome_pdf

#endif  // PDF_PDFIUM_PDFIUM_PAGE_H_
