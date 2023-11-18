#pragma once

#include <string>
#include <uriparser/Uri.h>


namespace uriparser {
  class Uri {
    public:
      Uri(std::string uri) : uri_(uri){
        UriParserStateA state_;
        state_.uri = &uriParse_;
        isValid_   = uriParseUriA(&state_, uri_.c_str()) == URI_SUCCESS;
      }

      ~Uri() { uriFreeUriMembersA(&uriParse_); }

      bool isValid() const { return isValid_; }

      std::string scheme()   const { return fromRange(uriParse_.scheme); }
      std::string host()     const { return fromRange(uriParse_.hostText); }
      std::string port()     const { return fromRange(uriParse_.portText); }
      std::string path()     const { return fromList(uriParse_.pathHead, "/"); }
      std::string resource() const { return fromTail(uriParse_.pathTail); }
      std::string query()    const { return fromRange(uriParse_.query); }
      std::string fragment() const { return fromRange(uriParse_.fragment); }

    private:
      std::string uri_;
      UriUriA   uriParse_;
      bool    isValid_;

      std::string fromRange(const UriTextRangeA & rng) const {
        return std::string(rng.first, rng.afterLast);
      }

      std::string fromTail(UriPathSegmentA *xs) const {
        UriPathSegmentStructA * tail(xs);
        return fromRange(tail->text);
      }

      std::string fromList(UriPathSegmentA *xs, const std::string & delim) const {
        UriPathSegmentStructA * head(xs);
        std::string accum;

        while (head){
          accum += delim + fromRange(head->text);
          head = head->next;
        }
        return accum;
      }
  };
}