#ifndef included_ALE_Sifter_hh
#define included_ALE_Sifter_hh

/*
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/composite_key.hpp>
*/
#include <iostream>

// ALE extensions

#ifndef  included_ALE_hh
#include <ALE.hh>
#endif

namespace ALE_X { 
  
  namespace SifterDef {
    // Defines the traits of a sequence representing a subset of a multi_index container Index_.
    // A sequence defines output (input in std terminology) iterators for traversing an Index_ object.
    // Upon dereferencing values are extracted from each result record using a ValueExtractor_ object.
    template <typename Index_, typename ValueExtractor_>
    struct IndexSequenceTraits {
      typedef Index_ index_type;
      class iterator_base {
      public:
        // Standard iterator typedefs
        typedef ValueExtractor_                        extractor_type;
        typedef std::input_iterator_tag                iterator_category;
        typedef typename extractor_type::result_type   value_type;
        typedef int                                    difference_type;
        typedef value_type*                            pointer;
        typedef value_type&                            reference;
        
        // Underlying iterator type
        typedef typename index_type::iterator          itor_type;
      protected:
        // Underlying iterator 
        itor_type      _itor;
        // Member extractor
        extractor_type _ex;
      public:
        iterator_base(itor_type itor) {
          this->_itor = itor_type(itor);
        };
        virtual ~iterator_base() {};
        virtual bool              operator==(const iterator_base& iter) const {return this->_itor == iter._itor;};
        virtual bool              operator!=(const iterator_base& iter) const {return this->_itor != iter._itor;};
        // FIX: operator*() should return a const reference, but it won't compile that way, because _ex() returns const value_type
        virtual const value_type  operator*() const {return _ex(*(this->_itor));};
      };// class iterator_base
      class iterator : public iterator_base {
      public:
        // Standard iterator typedefs
        typedef typename iterator_base::iterator_category  iterator_category;
        typedef typename iterator_base::value_type         value_type;
        typedef typename iterator_base::extractor_type     extractor_type;
        typedef typename iterator_base::difference_type    difference_type;
        typedef typename iterator_base::pointer            pointer;
        typedef typename iterator_base::reference          reference;
        // Underlying iterator type
        typedef typename iterator_base::itor_type          itor_type;
      public:
        iterator(const itor_type& itor) : iterator_base(itor) {};
        virtual ~iterator() {};
        //
        virtual iterator   operator++() {++this->_itor; return *this;};
        virtual iterator   operator++(int n) {iterator tmp(this->_itor); ++this->_itor; return tmp;};
      };// class iterator
    }; // struct IndexSequenceTraits
    
    template <typename Index_, typename ValueExtractor_>
    struct ReversibleIndexSequenceTraits {
      typedef IndexSequenceTraits<Index_, ValueExtractor_> base_traits;
      typedef typename base_traits::iterator_base   iterator_base;
      typedef typename base_traits::iterator        iterator;
      typedef typename base_traits::index_type      index_type;

      // reverse_iterator is the reverse of iterator
      class reverse_iterator : public iterator_base {
      public:
        // Standard iterator typedefs
        typedef typename iterator_base::iterator_category  iterator_category;
        typedef typename iterator_base::value_type         value_type;
        typedef typename iterator_base::extractor_type     extractor_type;
        typedef typename iterator_base::difference_type    difference_type;
        typedef typename iterator_base::pointer            pointer;
        typedef typename iterator_base::reference          reference;
        // Underlying iterator type
        typedef typename iterator_base::itor_type          itor_type;
      public:
        reverse_iterator(const itor_type& itor) : iterator_base(itor) {};
        virtual ~reverse_iterator() {};
        //
        virtual reverse_iterator     operator++() {--this->_itor; return *this;};
        virtual reverse_iterator     operator++(int n) {reverse_iterator tmp(this->_itor); --this->_itor; return tmp;};
      };
    }; // class ReversibleIndexSequenceTraits


    // 
    // Arrow & ArrowContainer definitions
    // 
    template<typename Source_, typename Target_, typename Color_>
    struct  Arrow { //: public ALE::def::Arrow<Source_, Target_, Color_> {
      typedef Arrow   arrow_type;
      typedef Source_ source_type;
      typedef Target_ target_type;
      typedef Color_  color_type;
      source_type source;
      target_type target;
      color_type  color;
      // Basic
      Arrow(const source_type& s, const target_type& t, const color_type& c) : source(s), target(t), color(c) {};
      // Flipping
      template <typename OtherSource_, typename OtherTarget_, typename OtherColor_>
      struct rebind {
        typedef Arrow<OtherSource_, OtherTarget_, OtherColor_> type;
      };
      struct flip {
        typedef Arrow<target_type, source_type, color_type> type;
        type arrow(const arrow_type& a) { return type(a.target, a.source, a.color);};
      };

      // Printing
      friend std::ostream& operator<<(std::ostream& os, const Arrow& a) {
        os << a.source << " --(" << a.color << ")--> " << a.target;
        return os;
      }

      // Arrow modifiers
      struct sourceChanger {
        sourceChanger(const source_type& newSource) : _newSource(newSource) {};
        void operator()(arrow_type& a) {a.source = this->_newSource;}
      private:
        source_type _newSource;
      };

      struct targetChanger {
        targetChanger(const target_type& newTarget) : _newTarget(newTarget) {};
        void operator()(arrow_type& a) { a.target = this->_newTarget;}
      private:
        const target_type _newTarget;
      };

      struct colorChanger {
        colorChanger(const color_type& newColor) : _newColor(newColor) {};
        void operator()(arrow_type& a) { a.color = this->_newColor;}
      private:
        const color_type _newColor;
      };
    };// struct Arrow
    

    template<typename Source_, typename Target_, typename Color_>
    struct ArrowContainerTraits {
    public:

    };// class ArrowContainerTraits
  
    //
    // Definitions of wrappers for comparison predicates:
    // The most basic predicates, called 'orders', for our purposes is a lexicographical order on one or two keys.
    //
    // lex1 simply delegates to the underlying Order_; defined purely for aesthetic purposes only
    template <typename Key_, typename Order_ = std::less<Key_> >
    struct lex1 {
    private:
      Order_ _less;
    public:
      bool operator()(const Key_& keyA, const Key_& keyB) 
      {
        return  (_less(keyA,keyB));
      };
    };
    //
    template <typename Key1_, typename Key2_,typename Order1_ = std::less<Key1_>, typename Order2_ = std::less<Key2_> >
    struct lex2 {
    private:
      Order1_ _less1;
      Order2_ _less2;
    public:
      bool operator()(const Key1_& key1A, const Key2_& key2A, const Key1_& key1B, const Key2_& key2B)
      {
        // In the following (key1A < key1B) || ((key1A == key1B)&&(key2A < key2B)) is computed.
        // Since we don't have equivalence '==' explicitly, it is defined by !(key1A < key1B) && !(key1B < key1A).
        // Furthermore, the expression to the right of '||' is evaluated only if that to the left of '||' fails (C semantics),
        // which means that !(key1A < key1B) is true, and we only need to test the other possibility to establish 
        // key equivalence key1A == key1B
        return  (_less1(key1A,key1B) || 
                 (!_less1(key1B,key1A) &&  _less2(key2A,key2B)) );
      };
    };

    //
    // Specializations of lex2 order to Arrow compare various combinations of pairs of Source, Target and Color in various orders.
    //
    //
    template<typename Arrow_, typename KeyExtractorA_, typename KeyExtractorB_, 
             typename OrderA_ = std::less<typename KeyExtractorA_::result_type>, 
             typename OrderB_ = std::less<typename KeyExtractorB_::result_type> >
    struct ArrowLex2Order {
      typedef typename KeyExtractorA_::result_type KeyA_;
      typedef typename KeyExtractorB_::result_type KeyB_;
      typedef ALE::pair<KeyA_,KeyB_> KeyAB_;
    private:
      KeyExtractorA_ _keyA;
      KeyExtractorB_ _keyB;
      lex1<KeyA_, OrderA_>                 _lex1;
      lex2<KeyA_, KeyB_, OrderA_, OrderB_> _lex2;
    public:
      bool operator()(const Arrow_& arr1, const Arrow_& arr2) {
        return _lex2(_keyA(arr1), _keyB(arr1), _keyA(arr2), _keyB(arr2));
      };
      bool operator()(const Arrow_& arr, const KeyA_ keyA) {
        return _lex1(_keyA(arr), keyA);
      };
      bool operator()(const Arrow_& arr, const KeyAB_ keyAB) {
        return _lex2(_keyA(arr), _keyB(arr), keyAB.first, keyAB.second);
      };
    };
    //
    template<typename Arrow_, 
             typename SourceOrder_ = std::less<typename Arrow_::source_type>,
             typename ColorOrder_  = std::less<typename Arrow_::color_type> >
    struct ArrowSourceColorOrder : 
      public ArrowLex2Order< ::boost::multi_index::template member<Arrow_, typename Arrow_::source_type, &Arrow_::source>, 
                             ::boost::multi_index::template member<Arrow_, typename Arrow_::color_type,  &Arrow_::color >,
                             SourceOrder_, ColorOrder_> {};
    //
    template<typename Arrow_, 
             typename ColorOrder_  = std::less<typename Arrow_::color_type>,
             typename SourceOrder_ = std::less<typename Arrow_::source_type> >
    struct ArrowColorSourceOrder : 
      public ArrowLex2Order< ::boost::multi_index::template member<Arrow_, typename Arrow_::color_type,  &Arrow_::color >,
                             ::boost::multi_index::template member<Arrow_, typename Arrow_::source_type, &Arrow_::source>, 
                             ColorOrder_, SourceOrder_> {};
    //
    template<typename Arrow_, 
             typename TargetOrder_ = std::less<typename Arrow_::target_type>,
             typename ColorOrder_  = std::less<typename Arrow_::color_type> >
    struct ArrowTargetColorOrder : 
      public ArrowLex2Order< Arrow_,
                             ::boost::multi_index::template member<Arrow_, typename Arrow_::target_type, &Arrow_::target>, 
                             ::boost::multi_index::template member<Arrow_, typename Arrow_::color_type, &Arrow_::color  >,
                             TargetOrder_, ColorOrder_> {};

    //
    template<typename Arrow_, 
             typename ColorOrder_  = std::less<typename Arrow_::color_type>,
             typename TargetOrder_ = std::less<typename Arrow_::target_type> >
    struct ArrowColorTargetOrder : 
      public ArrowLex2Order< Arrow_, 
                             ::boost::multi_index::template member<Arrow_, typename Arrow_::color_type,  &Arrow_::color >,
                             ::boost::multi_index::template member<Arrow_, typename Arrow_::target_type, &Arrow_::target>, 
                             ColorOrder_, TargetOrder_> {};
    //
    // Composite Arrow comparison operators, called 'compares', are used to generate cone and support indices.
    // A KeyArrowOrder first orders on a single key using KeyOrder (e.g., Target or Source for cone and support respectively),
    // and then on the whole Arrow, using an additional predicate ArrowOrder.
    // These are then specialized to SupportCompare & ConeCompare, using the order operators supplied by the user:
    // SupportCompare = (SourceOrder, SupportOrder), 
    // ConeCompare    = (TargetOrder, ConeOrder), etc
    template <typename Arrow_, typename KeyExtractor_, typename KeyOrder_, typename ArrowOrder_>
    struct KeyArrowCompare {
      typedef Arrow_                                                           arrow_type;
      typedef ArrowOrder_                                                      arrow_order_type;
      typedef KeyExtractor_                                                    key_extractor_type;
      typedef KeyOrder_                                                        key_order_type;
      typedef typename key_extractor_type::result_type                         key_type;
      //
      typedef lex2<key_type, arrow_type, key_order_type, arrow_order_type>     compare_type;
    private:
      compare_type       _compare;
      key_extractor_type _key;
    public:
      bool operator()(const arrow_type& arr1, const arrow_type& arr2) { 
        return _compare(_key(arr1),arr1,_key(arr2),arr2);
      };
    };

    //
    // Default orders & compares.
    //
    template <typename Arrow_>
    struct DefaultConeOrder : public ArrowColorSourceOrder<Arrow_> {};
    //
    template <typename Arrow_>
    struct DefaultSupportOrder : public ArrowColorTargetOrder<Arrow_> {};
    // 
    template <typename Arrow_>
    struct DefaultSourceOrder : public std::less<typename Arrow_::source_type> {};
    //
    template <typename Arrow_>
    struct DefaultTargetOrder : public std::less<typename Arrow_::target_type> {};
    //
    template <typename Arrow_>
    struct DefaultColorOrder : public std::less<typename Arrow_::color_type> {};
    //
    //
    // KeyArrowCompare specialized for support and cone indices, parameterized by the comparison predicates.
    template <typename Arrow_, typename SourceOrder_=DefaultSourceOrder<Arrow_>, typename SupportOrder_=DefaultSupportOrder<Arrow_> >
    struct SupportCompare : 
      public KeyArrowCompare<Arrow_,typename ::boost::multi_index::template member<Arrow_,typename Arrow_::source_type,&Arrow_::source>,
                             SourceOrder_, SupportOrder_>{};
    //
    template <typename Arrow_, typename TargetOrder_=DefaultTargetOrder<Arrow_>, typename ConeOrder_=DefaultConeOrder<Arrow_> >
    struct ConeCompare: 
      public KeyArrowCompare<Arrow_, typename ::boost::multi_index::member<Arrow_,typename Arrow_::target_type,&Arrow_::target>, 
                             TargetOrder_, ConeOrder_>{};
    
    
    //
    // ArrowContainer definition
    template<typename Arrow_, typename SupportOrder_, typename ConeOrder_>
    struct ArrowContainer {
      //
      // Encapsulated types
      //
      typedef Arrow_                           arrow_type;
      typedef typename arrow_type::source_type source_type;
      typedef typename arrow_type::target_type target_type;
      typedef typename arrow_type::color_type  color_type;
      // Index tags
      struct                                   SupportTag{};
      struct                                   ConeTag{};
      //
      // Arrow set type
      typedef ::boost::multi_index::multi_index_container<
        arrow_type,
        ::boost::multi_index::indexed_by<
          ::boost::multi_index::ordered_non_unique<
            ::boost::multi_index::tag<SupportTag>,::boost::multi_index::identity<arrow_type>, SupportCompare<arrow_type>
          >,
          ::boost::multi_index::ordered_non_unique<
            ::boost::multi_index::tag<ConeTag>,::boost::multi_index::identity<arrow_type>, ConeCompare<arrow_type>
          >
        >,
        ALE_ALLOCATOR<arrow_type>
      > arrow_set_type;
      //
      // Sequence types
      template <typename Index_, typename Key_, typename SubKey_, typename ValueExtractor_>
      class ArrowSequence {
        // ArrowSequence implements ReversibleIndexSequencTraits with Index_ and ValueExtractor_ types.
        // A Key_ object and an optional SubKey_ object are used to extract the index subset.
      public:
        typedef ReversibleIndexSequenceTraits<Index_, ValueExtractor_>  traits;
        typedef Key_                                                    key_type;
        typedef SubKey_                                                 subkey_type;
      protected:
        typename traits::index_type&                                    _index;
        const key_type                                                  key;
        const subkey_type                                               subkey;
        const bool                                                      useSubkey;
      public:
        // Need to extend the inherited iterators to be able to extract arrow color
        class iterator : public traits::iterator {
        public:
          iterator(const typename traits::iterator::itor_type& itor) : traits::iterator(itor) {};
          virtual const source_type& source() const {return this->_itor->source;};
          virtual const color_type&  color()  const {return this->_itor->color;};
          virtual const target_type& target() const {return this->_itor->target;};
          virtual const arrow_type&  arrow()  const {return *(this->_itor);};
        };
        class reverse_iterator : public traits::reverse_iterator {
        public:
          reverse_iterator(const typename traits::reverse_iterator::itor_type& itor) : traits::reverse_iterator(itor) {};
          virtual const source_type& source() const {return this->_itor->source;};
          virtual const color_type&  color()  const {return this->_itor->color;};
          virtual const target_type& target() const {return this->_itor->target;};
          virtual const arrow_type&  arrow()  const {return *(this->_itor);};
        };
      public:
        //
        // Basic ArrowSequence interface
        //
        ArrowSequence(const ArrowSequence& seq) : _index(seq._index), key(seq.key), subkey(seq.subkey), useSubkey(seq.useSubkey) {};
        ArrowSequence(typename traits::index_type& index, const key_type& k) : 
          _index(index), key(k), subkey(subkey_type()), useSubkey(0) {};
        ArrowSequence(typename traits::index_type& index, const key_type& k, const subkey_type& kk) : 
          _index(index), key(k), subkey(kk), useSubkey(1){};
        virtual ~ArrowSequence() {};
        //
        // Extended ArrowSequence interface
        //
        virtual bool         empty() {return this->_index.empty();};
        //
        virtual typename traits::index_type::size_type  size()  {
          if (this->useSubkey) {
            return this->_index.count(::boost::make_tuple(this->key,this->subkey));
          } else {
            return this->_index.count(::boost::make_tuple(this->key));
          }
        };
        //
        virtual iterator begin() {
          if (this->useSubkey) {
            return iterator(this->_index.lower_bound(::boost::make_tuple(this->key,this->subkey)));
          } else {
            return iterator(this->_index.lower_bound(::boost::make_tuple(this->key)));
          }
        };
        //
        virtual iterator end() {
          if (this->useSubkey) {
            return iterator(this->_index.upper_bound(::boost::make_tuple(this->key,this->subkey)));
          } else {
            return iterator(this->_index.upper_bound(::boost::make_tuple(this->key)));
          }
        };
        //
        virtual reverse_iterator rbegin() {
          if (this->useSubkey) {
            return reverse_iterator(--this->_index.upper_bound(::boost::make_tuple(this->key,this->subkey)));
          } else {
            return reverse_iterator(--this->_index.upper_bound(::boost::make_tuple(this->key)));
          }
        };
        //
        virtual reverse_iterator rend() {
          if (this->useSubkey) {
            return reverse_iterator(--this->_index.lower_bound(::boost::make_tuple(this->key,this->subkey)));
          } else {
            return reverse_iterator(--this->_index.lower_bound(::boost::make_tuple(this->key)));
          }
        };
        //
        template<typename ostream_type>
        void view(ostream_type& os, const bool& useColor = false, const char* label = NULL){
          if(label != NULL) {
            os << "Viewing " << label << " sequence:" << std::endl;
          } 
          os << "[";
          for(iterator i = this->begin(); i != this->end(); i++) {
            os << " (" << *i;
            if(useColor) {
              os << "," << i.color();
            }
            os  << ")";
          }
          os << " ]" << std::endl;
        };
        void addArrow(const source_type& s, const color_type& c) {
          this->_graph.addArrow(arrow_type(s,this->key,c));
        };
        
        virtual bool contains(const source_type& s) {
          // Check whether a given point is in the index
          typename ::boost::multi_index::index<typename ASifter::traits::arrow_container_type::set_type,typename ASifter::traits::arrowInd>::type& index = ::boost::multi_index::get<typename ASifter::traits::arrowInd>(this->_graph._arrows.set);
          return (index.find(::boost::make_tuple(s,this->key)) != index.end());
        };
      };// class ArrowSequence    
      //
      // multi-index set of multicolor arrows
      arrow_set_type arrow_set;
      
    }; // class ArrowContainer
    
  }; // namespace SifterDef


  //
  // Sifter (structurally a bipartite graph with colored arrows) implements a sequential interface 
  // similar to that of Sieve, except the source and target points may have different types and iterated operations (e.g., nCone, 
  // closure) are not available.
  // 
  template<typename Arrow_, 
           typename SupportOrder_ = SifterDef::DefaultSupportOrder<Arrow_>,
           typename ConeOrder_    = SifterDef::DefaultConeOrder<Arrow_>    
  >
  class Sifter { // class Sifter
  public:
    // Encapsulated container types
    typedef SifterDef::ArrowContainer<Arrow_, SupportOrder_, ConeOrder_>           arrow_container_type;
    // Types associated with records held in containers
    typedef Arrow_                                                                 arrow_type;
    typedef typename arrow_type::source_type                                       source_type;
    typedef typename arrow_type::target_type                                       target_type;
    typedef SupportOrder_                                                          support_order_type;
    typedef ConeOrder_                                                             cone_order_type;
    // Convenient tag names
    typedef typename arrow_container_type::SupportTag                              supportInd;
    typedef typename arrow_container_type::ConeTag                                 coneInd;
    //
    // Return types
    //
    // FIX: This is a temp fix to include addArrow into the interface; should probably be pushed up to ArrowSequence
    struct coneSequence : public arrow_container_type::traits::template ArrowSequence<typename ::boost::multi_index::index<typename arrow_container_type::set_type,coneInd>::type, target_type, color_type, BOOST_MULTI_INDEX_MEMBER(arrow_type, source_type, source)> {
    protected:
      graph_type& _graph;
    public:
      typedef typename 
        arrow_container_type::traits::template ArrowSequence<typename ::boost::multi_index::index<typename arrow_container_type::set_type,coneInd>::type, target_type, color_type, BOOST_MULTI_INDEX_MEMBER(arrow_type, source_type, source)> base_type;
      // Encapsulated types
      typedef typename base_type::traits traits;
      typedef typename base_type::iterator iterator;
      typedef typename base_type::reverse_iterator reverse_iterator;
      // Basic interface
      coneSequence(const coneSequence& seq) : base_type(seq), _graph(seq._graph) {};
      coneSequence(graph_type& graph, typename traits::index_type& index, const typename base_type::key_type& k) : base_type(index, k), _graph(graph){};
      coneSequence(graph_type& graph, typename traits::index_type& index, const typename base_type::key_type& k, const typename base_type::subkey_type& kk) : base_type(index, k, kk), _graph(graph) {};
      virtual ~coneSequence() {};
            
      // Extended interface
      void addArrow(const arrow_type& a) {
        // if(a.target != this->key) {
        //               throw ALE::Exception("Arrow target mismatch in a coneSequence");
        //             }
        this->_graph.addArrow(a);
      };
      void addArrow(const source_type& s, const color_type& c){
        this->_graph.addArrow(arrow_type(s,this->key,c));
      };
      
      virtual bool contains(const source_type& s) {
        // Check whether a given point is in the index
        typename ::boost::multi_index::index<typename ASifter::traits::arrow_container_type::set_type,typename ASifter::traits::arrowInd>::type& index = ::boost::multi_index::get<typename ASifter::traits::arrowInd>(this->_graph._arrows.set);
        return (index.find(::boost::make_tuple(s,this->key)) != index.end());
      };
    };// struct coneSequence
      
    // FIX: This is a temp fix to include addArrow into the interface; should probably be pushed up to ArrowSequence
    struct supportSequence : public arrow_container_type::traits::template ArrowSequence<typename ::boost::multi_index::index<typename arrow_container_type::set_type,supportInd>::type, source_type, color_type, BOOST_MULTI_INDEX_MEMBER(arrow_type, target_type, target)> {
    protected:
      graph_type& _graph;
    public:
      typedef typename 
      arrow_container_type::template ArrowSequence<typename ::boost::multi_index::index<typename arrow_container_type::set_type,supportInd>::type, source_type, color_type, BOOST_MULTI_INDEX_MEMBER(arrow_type, target_type, target)> base_type;
      // Encapsulated types
      typedef typename base_type::traits traits;
      typedef typename base_type::iterator iterator;
      typedef typename base_type::reverse_iterator reverse_iterator;
      // Basic interface
      supportSequence(const supportSequence& seq) : base_type(seq), _graph(seq._graph) {};
        supportSequence(graph_type& graph, typename traits::index_type& index, const typename base_type::key_type& k) : base_type(index, k), _graph(graph){};
          supportSequence(graph_type& graph, typename traits::index_type& index, const typename base_type::key_type& k, const typename base_type::subkey_type& kk) : base_type(index, k, kk), _graph(graph) {};
            virtual ~supportSequence() {};
            
            // FIX: WARNING: (or a HACK?): we flip the arrow on addition here. 
            // Fancy interface
            void addArrow(const typename arrow_type::flip::type& af) {
              this->_graph.addArrow(af.target, af.source, af.color);
            };
            void addArrow(const target_type& t, const color_type& c){
              this->_graph.addArrow(arrow_type(this->key,t,c));
            };
    };// struct supportSequence

    typedef ALE::set<source_type>   coneSet;
    typedef ALE::array<source_type> coneArray;
    typedef ALE::set<target_type>   supportSet;
    typedef ALE::array<target_type> supportArray;

    template <typename OtherArrow_, 
              OtherSupportOrder_ = support_order_type::template rebind<OtherArrow_>::type,
              OtherConeOrder_    = cone_order_type::template rebind<OtherArrow_>::type
    >
    struct rebind {
      typedef Sifter<OtherArrow_, OtherSupportOrder_, OtherConeOrder_> type;
    };

  public:
    // Debug level
    int debug;
    //protected:
    typename arrow_container_type _arrows;
  protected:
    MPI_Comm    _comm;
    int         _commRank;
    int         _commSize;
    PetscObject _petscObj;
    void __init(MPI_Comm comm) {    
      PetscErrorCode ierr;
      this->_comm = comm;
      ierr = MPI_Comm_rank(this->_comm, &this->_commRank); CHKERROR(ierr, "Error in MPI_Comm_rank");
      ierr = MPI_Comm_size(this->_comm, &this->_commSize); CHKERROR(ierr, "Error in MPI_Comm_rank"); 
      ierr = PetscObjectCreate(this->_comm, &this->_petscObj); CHKERROR(ierr, "Failed in PetscObjectCreate");
    };
  public:
    // 
    // Basic interface
    //
    Sifter(MPI_Comm comm = PETSC_COMM_SELF, const int& debug = 0) {__init(comm);  this->debug = debug;}
    virtual ~Sifter(){};
    //
    // Query methods
    //
    MPI_Comm    comm()     const {return this->_comm;};
    int         commSize() const {return this->_commSize;};
    int         commRank() const {return this->_commRank;}
    PetscObject petscObj() const {return this->_petscObj;};

    // FIX: need const_cap, const_base returning const capSequence etc, but those need to have const_iterators, const_begin etc.
    Obj<typename traits::capSequence> cap() {
      return typename traits::capSequence(::boost::multi_index::get<typename traits::capInd>(this->_cap.set));
    };
    Obj<typename traits::baseSequence> base() {
      return typename traits::baseSequence(::boost::multi_index::get<typename traits::baseInd>(this->_base.set));
    };
    // FIX: should probably have cone and const_cone etc, since arrows can be modified through an iterator (modifyColor).
    Obj<typename traits::arrowSequence> 
    arrows(const typename traits::source_type& s, const typename traits::target_type& t) {
      return typename traits::arrowSequence(::boost::multi_index::get<typename traits::arrowInd>(this->_arrows.set), s, t);
    };
    Obj<typename traits::arrowSequence> 
    arrows(const typename traits::source_type& s) {
      return typename traits::arrowSequence(::boost::multi_index::get<typename traits::arrowInd>(this->_arrows.set), s);
    };
    Obj<typename traits::coneSequence> 
    cone(const typename traits::target_type& p) {
      return typename traits::coneSequence(*this, ::boost::multi_index::get<typename traits::coneInd>(this->_arrows.set), p);
    };
    template<class InputSequence> 
    Obj<typename traits::coneSet> 
    cone(const Obj<InputSequence>& points) {
      return this->cone(points, typename traits::color_type(), false);
    };
    Obj<typename traits::coneSequence> 
    cone(const typename traits::target_type& p, const typename traits::color_type& color) {
      return typename traits::coneSequence(*this, ::boost::multi_index::get<typename traits::coneInd>(this->_arrows.set), p, color);
    };
    template<class InputSequence>
    Obj<typename traits::coneSet> 
    cone(const Obj<InputSequence>& points, const typename traits::color_type& color, bool useColor = true) {
      Obj<typename traits::coneSet> cone = typename traits::coneSet();
      for(typename InputSequence::iterator p_itor = points->begin(); p_itor != points->end(); ++p_itor) {
        Obj<typename traits::coneSequence> pCone;
        if (useColor) {
          pCone = this->cone(*p_itor, color);
        } else {
          pCone = this->cone(*p_itor);
        }
        cone->insert(pCone->begin(), pCone->end());
      }
      return cone;
    };
    template<typename PointCheck>
    bool coneContains(const typename traits::target_type& p, const PointCheck& checker) {
      typename traits::coneSequence cone(*this, ::boost::multi_index::get<typename traits::coneInd>(this->_arrows.set), p);

      for(typename traits::coneSequence::iterator c_iter = cone.begin(); c_iter != cone.end(); ++c_iter) {
        if (checker(*c_iter, p)) return true;
      }
      return false;
    };
    template<typename PointProcess>
    void coneApply(const typename traits::target_type& p, PointProcess& processor) {
      typename traits::coneSequence cone(*this, ::boost::multi_index::get<typename traits::coneInd>(this->_arrows.set), p);

      for(typename traits::coneSequence::iterator c_iter = cone.begin(); c_iter != cone.end(); ++c_iter) {
        processor(*c_iter, p);
      }
    };
    Obj<typename traits::supportSequence> 
    support(const typename traits::source_type& p) {
      return typename traits::supportSequence(*this, ::boost::multi_index::get<typename traits::supportInd>(this->_arrows.set), p);
    };
    Obj<typename traits::supportSequence> 
    support(const typename traits::source_type& p, const typename traits::color_type& color) {
      return typename traits::supportSequence(*this, ::boost::multi_index::get<typename traits::supportInd>(this->_arrows.set), p, color);
    };

    template<class InputSequence>
    Obj<typename traits::supportSet>      
    support(const Obj<InputSequence>& sources) {
      return this->support(sources, typename traits::color_type(), false);
    };

    template<class InputSequence>
    Obj<typename traits::supportSet>      
    support(const Obj<InputSequence>& points, const typename traits::color_type& color, bool useColor = true){
      Obj<typename traits::supportSet> supp = typename traits::supportSet();
      for(typename InputSequence::iterator p_itor = points->begin(); p_itor != points->end(); ++p_itor) {
        Obj<typename traits::supportSequence> pSupport;
        if (useColor) {
          pSupport = this->support(*p_itor, color);
        } else {
          pSupport = this->support(*p_itor);
        }
        supp->insert(pSupport->begin(), pSupport->end());
      }
      return supp;
    };

    template<typename ostream_type>
    void view(ostream_type& os, const char* label = NULL, bool rawData = false){
      if(label != NULL) {
        os << "Viewing Sifter '" << label << "':" << std::endl;
      } 
      else {
        os << "Viewing a Sifter:" << std::endl;
      }
      if(!rawData) {
        os << "cap --> base:" << std::endl;
        typename traits::capSequence cap = this->cap();
        for(typename traits::capSequence::iterator capi = cap.begin(); capi != cap.end(); capi++) {
          typename traits::supportSequence supp = this->support(*capi);
          for(typename traits::supportSequence::iterator suppi = supp.begin(); suppi != supp.end(); suppi++) {
            os << *capi << "--(" << suppi.color() << ")-->" << *suppi << std::endl;
          }
        }
        os << "base <-- cap:" << std::endl;
        typename traits::baseSequence base = this->base();
        for(typename traits::baseSequence::iterator basei = base.begin(); basei != base.end(); basei++) {
          typename traits::coneSequence cone = this->cone(*basei);
          for(typename traits::coneSequence::iterator conei = cone.begin(); conei != cone.end(); conei++) {
            os << *basei <<  "<--(" << conei.color() << ")--" << *conei << std::endl;
          }
        }
        os << "cap --> outdegrees:" << std::endl;
        for(typename traits::capSequence::iterator capi = cap.begin(); capi != cap.end(); capi++) {
          os << *capi <<  "-->" << capi.degree() << std::endl;
        }
        os << "base <-- indegrees:" << std::endl;
        for(typename traits::baseSequence::iterator basei = base.begin(); basei != base.end(); basei++) {
          os << *basei <<  "<--" << basei.degree() << std::endl;
        }
      }
      else {
        os << "'raw' arrow set:" << std::endl;
        for(typename traits::arrow_container_type::set_type::iterator ai = _arrows.set.begin(); ai != _arrows.set.end(); ai++)
        {
          typename traits::arrow_type arr = *ai;
          os << arr << std::endl;
        }
        os << "'raw' base set:" << std::endl;
        for(typename traits::base_container_type::set_type::iterator bi = _base.set.begin(); bi != _base.set.end(); bi++) 
        {
          typename traits::base_container_type::traits::rec_type bp = *bi;
          os << bp << std::endl;
        }
        os << "'raw' cap set:" << std::endl;
        for(typename traits::cap_container_type::set_type::iterator ci = _cap.set.begin(); ci != _cap.set.end(); ci++) 
        {
          typename traits::cap_container_type::traits::rec_type cp = *ci;
          os << cp << std::endl;
        }
      }
    };
    // A parallel viewer
    PetscErrorCode view(const char* label = NULL, bool raw = false){
      PetscErrorCode ierr;
      ostringstream txt;
      PetscFunctionBegin;
      if(debug) {
        std::cout << "viewing a Sifter, comm = " << this->comm() << ", PETSC_COMM_SELF = " << PETSC_COMM_SELF << ", commRank = " << this->commRank() << std::endl;
      }
      if(label != NULL) {
        if(this->commRank() == 0) {
          txt << "viewing Sifter :'" << label << "'" << std::endl;
        }
      } 
      else {
        if(this->commRank() == 0) {
          txt << "viewing a Sifter" << std::endl;
        }
      }
      ierr = PetscSynchronizedPrintf(this->comm(), txt.str().c_str()); CHKERROR(ierr, "Error in PetscSynchronizedFlush");
      ierr = PetscSynchronizedFlush(this->comm());  CHKERROR(ierr, "Error in PetscSynchronizedFlush");
      if(!raw) {
        ostringstream txt;
        if(this->commRank() == 0) {
          txt << "cap --> base:\n";
        }
        typename traits::capSequence cap   = this->cap();
        typename traits::baseSequence base = this->base();
        if(cap.empty()) {
          txt << "[" << this->commRank() << "]: empty" << std::endl; 
        }
        for(typename traits::capSequence::iterator capi = cap.begin(); capi != cap.end(); capi++) {
          typename traits::supportSequence supp = this->support(*capi);
          for(typename traits::supportSequence::iterator suppi = supp.begin(); suppi != supp.end(); suppi++) {
            txt << "[" << this->commRank() << "]: " << *capi << "--(" << suppi.color() << ")-->" << *suppi << std::endl;
          }
        }
        //
        ierr = PetscSynchronizedPrintf(this->comm(), txt.str().c_str()); CHKERROR(ierr, "Error in PetscSynchronizedFlush");
        ierr = PetscSynchronizedFlush(this->comm());  CHKERROR(ierr, "Error in PetscSynchronizedFlush");
        //
        ostringstream txt1;
        if(this->commRank() == 0) {
          //txt1 << "cap <point,degree>:\n";
          txt1 << "cap:\n";
        }
        txt1 << "[" << this->commRank() << "]:  [";
        for(typename traits::capSequence::iterator capi = cap.begin(); capi != cap.end(); capi++) {
          //txt1 << " <" << *capi << "," << capi.degree() << ">";
          txt1 << "  " << *capi;
        }
        txt1 << " ]" << std::endl;
        //
        ierr = PetscSynchronizedPrintf(this->comm(), txt1.str().c_str()); CHKERROR(ierr, "Error in PetscSynchronizedFlush");
        ierr = PetscSynchronizedFlush(this->comm());  CHKERROR(ierr, "Error in PetscSynchronizedFlush");
        //
        ostringstream txt2;
        if(this->commRank() == 0) {
          //txt2 << "base <point,degree>:\n";
          txt2 << "base:\n";
        }
        txt2 << "[" << this->commRank() << "]:  [";
        for(typename traits::baseSequence::iterator basei = base.begin(); basei != base.end(); basei++) {
          txt2 << "  " << *basei;
          //txt2 << " <" << *basei << "," << basei.degree() << ">";
        }
        txt2 << " ]" << std::endl;
        //
        ierr = PetscSynchronizedPrintf(this->comm(), txt2.str().c_str()); CHKERROR(ierr, "Error in PetscSynchronizedFlush");
        ierr = PetscSynchronizedFlush(this->comm());  CHKERROR(ierr, "Error in PetscSynchronizedFlush");
      }
      else { // if(raw)
        ostringstream txt;
        if(this->commRank() == 0) {
          txt << "'raw' arrow set:" << std::endl;
        }
        for(typename traits::arrow_container_type::set_type::iterator ai = _arrows.set.begin(); ai != _arrows.set.end(); ai++)
        {
          typename traits::arrow_type arr = *ai;
          txt << "[" << this->commRank() << "]: " << arr << std::endl;
        }
        ierr = PetscSynchronizedPrintf(this->comm(), txt.str().c_str()); CHKERROR(ierr, "Error in PetscSynchronizedFlush");
        ierr = PetscSynchronizedFlush(this->comm());  CHKERROR(ierr, "Error in PetscSynchronizedFlush");
        //
        ostringstream txt1;
        if(this->commRank() == 0) {
          txt1 << "'raw' base set:" << std::endl;
        }
        for(typename traits::base_container_type::set_type::iterator bi = _base.set.begin(); bi != _base.set.end(); bi++) 
        {
          typename traits::base_container_type::traits::rec_type bp = *bi;
          txt1 << "[" << this->commRank() << "]: " << bp << std::endl;
        }
        ierr = PetscSynchronizedPrintf(this->comm(), txt1.str().c_str()); CHKERROR(ierr, "Error in PetscSynchronizedFlush");
        ierr = PetscSynchronizedFlush(this->comm());  CHKERROR(ierr, "Error in PetscSynchronizedFlush");
        //
        ostringstream txt2;
        if(this->commRank() == 0) {
          txt2 << "'raw' cap set:" << std::endl;
        }
        for(typename traits::cap_container_type::set_type::iterator ci = _cap.set.begin(); ci != _cap.set.end(); ci++) 
        {
          typename traits::cap_container_type::traits::rec_type cp = *ci;
          txt2 << "[" << this->commRank() << "]: " << cp << std::endl;
        }
        ierr = PetscSynchronizedPrintf(this->comm(), txt2.str().c_str()); CHKERROR(ierr, "Error in PetscSynchronizedFlush");
        ierr = PetscSynchronizedFlush(this->comm());  CHKERROR(ierr, "Error in PetscSynchronizedFlush");
      }// if(raw)
      
      PetscFunctionReturn(0);
    };
  public:
    //
    // Lattice queries
    //
    template<class targetInputSequence> 
    Obj<typename traits::coneSequence> meet(const Obj<targetInputSequence>& targets);
    // unimplemented
    template<class targetInputSequence> 
    Obj<typename traits::coneSequence> meet(const Obj<targetInputSequence>& targets, const typename traits::color_type& color);
    // unimplemented
    template<class sourceInputSequence> 
    Obj<typename traits::coneSequence> join(const Obj<sourceInputSequence>& sources);
    // unimplemented
    template<class sourceInputSequence> 
    Obj<typename traits::coneSequence> join(const Obj<sourceInputSequence>& sources, const typename traits::color_type& color);
  public:
    //
    // Structural manipulation
    //
    void clear() {
      this->_arrows.set.clear(); this->_base.set.clear(); this->_cap.set.clear();
    };
    void addBasePoint(const typename traits::target_type t) {
      /* // Increase degree by 0, which won't affect an existing point and will insert a new point, if necessery
         this->_base.adjustDegree(t,0); */
      this->_base.set.insert(typename traits::targetRec_type(t,0));
    };
    void addBasePoint(const typename traits::targetRec_type b) {
      this->_base.set.insert(b);
    };
    void removeBasePoint(const typename traits::target_type t) {
      if (debug) {std::cout << " Removing " << t << " from the base" << std::endl;}
      // Clear the cone and remove the point from _base
      this->clearCone(t);
      this->_base.removePoint(t);
    };
    void addCapPoint(const typename traits::source_type s) {
      /* // Increase degree by 0, which won't affect an existing point and will insert a new point, if necessery
         this->_cap.adjustDegree(s,0); */
      this->_cap.set.insert(typename traits::sourceRec_type(s,0));
    };
    void addCapPoint(const typename traits::sourceRec_type c) {
      this->_cap.set.insert(c);
    };
    void removeCapPoint(const typename traits::source_type s) {
      if (debug) {std::cout << " Removing " << s << " from the cap" << std::endl;}
      // Clear the support and remove the point from _cap
      this->clearSupport(s);
      this->_cap.removePoint(s);
    };
    virtual void addArrow(const typename traits::source_type& p, const typename traits::target_type& q) {
      this->addArrow(p, q, typename traits::color_type());
    };
    virtual void addArrow(const typename traits::source_type& p, const typename traits::target_type& q, const typename traits::color_type& color) {
      this->addArrow(typename traits::arrow_type(p, q, color));
      //std::cout << "Added " << arrow_type(p, q, color);
    };
    virtual bool checkArrow(const typename traits::arrow_type& a) {
      if (this->_cap.set.find(a.source) == this->_cap.set.end()) return false;
      if (this->_base.set.find(a.target) == this->_base.set.end()) return false;
      return true;
    };
    virtual void addArrow(const typename traits::arrow_type& a, bool restrict = false) {
      if (restrict && !this->checkArrow(a)) return;
      this->_arrows.set.insert(a);
      this->addBasePoint(a.target);
      this->addCapPoint(a.source);
    };
    virtual void removeArrow(const typename traits::arrow_type& a) {
      // First, produce an arrow sequence for the given source, target combination.
      typename traits::arrowSequence::traits::index_type& arrowIndex = 
        ::boost::multi_index::get<typename traits::arrowInd>(this->_arrows.set);
      typename traits::arrowSequence::traits::index_type::iterator i,ii,j;
      i = arrowIndex.lower_bound(::boost::make_tuple(a.source,a.target));
      ii = arrowIndex.upper_bound(::boost::make_tuple(a.source, a.target));
      if(debug) { // if(debug)
        std::cout << "removeArrow: attempting to remove arrow:" << a << std::endl;
        std::cout << "removeArrow: candidate arrows are:" << std::endl;
      }
      for(j = i; j != ii; j++) {
        if(debug) { // if(debug)
          std::cout << " " << *j;
        }
        // Find the arrow of right color and remove it
        if(j->color == a.color) {
          if(debug) { // if(debug)
            std::cout << std::endl << "removeArrow: found:" << *j << std::endl;
          }
          /* this->_base.adjustDegree(a.target, -1); this->_cap.adjustDegree(a.source,-1); */
          arrowIndex.erase(j);
          break;
        }
      }
    };

    void addCone(const typename traits::source_type& source, const typename traits::target_type& target){
      this->addArrow(source, target);
    };
    template<class sourceInputSequence> 
    void addCone(const Obj<sourceInputSequence>& sources, const typename traits::target_type& target) {
      this->addCone(sources, target, typename traits::color_type());
    };
    void addCone(const typename traits::source_type& source, const typename traits::target_type& target, const typename traits::color_type& color) {
      this->addArrow(source, target, color);
    };
    template<class sourceInputSequence> 
    void 
    addCone(const Obj<sourceInputSequence>& sources, const typename traits::target_type& target, const typename traits::color_type& color){
      if (debug) {std::cout << "Adding a cone " << std::endl;}
      for(typename sourceInputSequence::iterator iter = sources->begin(); iter != sources->end(); ++iter) {
        if (debug) {std::cout << "Adding arrow from " << *iter << " to " << target << "(" << color << ")" << std::endl;}
        this->addArrow(*iter, target, color);
      }
    };
    void clearCone(const typename traits::target_type& t) {
      clearCone(t, typename traits::color_type(), false);
    };

    void clearCone(const typename traits::target_type& t, const typename traits::color_type&  color, bool useColor = true) {
      // Use the cone sequence types to clear the cone
      typename traits::coneSequence::traits::index_type& coneIndex = 
        ::boost::multi_index::get<typename traits::coneInd>(this->_arrows.set);
      typename traits::coneSequence::traits::index_type::iterator i, ii, j;
      if(debug) { // if(debug)
        std::cout << "clearCone: removing cone over " << t;
        if(useColor) {
          std::cout << " with color" << color << std::endl;
          typename traits::coneSequence cone = this->cone(t,color);
          std::cout << "[";
          for(typename traits::coneSequence::iterator ci = cone.begin(); ci != cone.end(); ci++) {
            std::cout << "  " << ci.arrow();
          }
          std::cout << "]" << std::endl;
        }
        else {
          std::cout << std::endl;
          typename traits::coneSequence cone = this->cone(t);
          std::cout << "[";
          for(typename traits::coneSequence::iterator ci = cone.begin(); ci != cone.end(); ci++) {
            std::cout << "  " << ci.arrow();
          }
          std::cout << "]" << std::endl;
        }
      }// if(debug)
      if (useColor) {
        i = coneIndex.lower_bound(::boost::make_tuple(t,color));
        ii = coneIndex.upper_bound(::boost::make_tuple(t,color));
      } else {
        i = coneIndex.lower_bound(::boost::make_tuple(t));
        ii = coneIndex.upper_bound(::boost::make_tuple(t));
      }
      for(j = i; j != ii; j++){          
        // Adjust the degrees before removing the arrow; use a different iterator, since we'll need i,ii to do the arrow erasing.
        if(debug) {
          std::cout << "clearCone: adjusting degrees for endpoints of arrow: " << *j << std::endl;
        }
        /* this->_cap.adjustDegree(j->source, -1);
           this->_base.adjustDegree(j->target, -1); */
      }
      coneIndex.erase(i,ii);
    };// clearCone()

    template<class InputSequence>
    void
    restrictBase(const Obj<InputSequence>& points) {
      typename traits::baseSequence base = this->base();
      typename std::set<typename traits::target_type> remove;
      
      for(typename traits::baseSequence::iterator bi = base.begin(); bi != base.end(); bi++) {
        // Check whether *bi is in points, if it is NOT, remove it
        //           if (!points->contains(*bi)) {
        if (points->find(*bi) == points->end()) {
          //             this->removeBasePoint(*bi);
          remove.insert(*bi);
        }
      }
      //FIX
      for(typename std::set<typename traits::target_type>::iterator r_iter = remove.begin(); r_iter != remove.end(); ++r_iter) {
        this->removeBasePoint(*r_iter);
      }
    };

    template<class InputSequence>
    void
    excludeBase(const Obj<InputSequence>& points) {
      for(typename InputSequence::iterator pi = points->begin(); pi != points->end(); pi++) {
        this->removeBasePoint(*pi);
      }
    };

    template<class InputSequence>
    void
    restrictCap(const Obj<InputSequence>& points) {
      typename traits::capSequence cap = this->cap();
      for(typename traits::capSequence::iterator ci = cap.begin(); ci != cap.end(); ci++) {
        // Check whether *ci is in points, if it is NOT, remove it
        if(points->find(*ci) == points->end()) {
          this->removeCapPoint(*ci);
        }
      }
    };

    template<class InputSequence>
    void
    excludeCap(const Obj<InputSequence>& points) {
      for(typename InputSequence::iterator pi = points->begin(); pi != points->end(); pi++) {
        this->removeCapPoint(*pi);
      }
    };

    void clearSupport(const typename traits::source_type& s) {
      clearSupport(s, typename traits::color_type(), false);
    };
    void clearSupport(const typename traits::source_type& s, const typename traits::color_type&  color, bool useColor = true) {
      // Use the cone sequence types to clear the cone
      typename 
        traits::supportSequence::traits::index_type& suppIndex = ::boost::multi_index::get<typename traits::supportInd>(this->_arrows.set);
      typename traits::supportSequence::traits::index_type::iterator i, ii, j;
      if (useColor) {
        i = suppIndex.lower_bound(::boost::make_tuple(s,color));
        ii = suppIndex.upper_bound(::boost::make_tuple(s,color));
      } else {
        i = suppIndex.lower_bound(::boost::make_tuple(s));
        ii = suppIndex.upper_bound(::boost::make_tuple(s));
      }
      for(j = i; j != ii; j++){
        // Adjust the degrees before removing the arrow
        /* this->_cap.adjustDegree(j->source, -1);
           this->_base.adjustDegree(j->target, -1); */
      }
      suppIndex.erase(i,ii);
    };
    void setCone(const typename traits::source_type& source, const typename traits::target_type& target){
      this->clearCone(target, typename traits::color_type(), false); this->addCone(source, target);
    };
    template<class sourceInputSequence> 
    void setCone(const Obj<sourceInputSequence>& sources, const typename traits::target_type& target) {
      this->clearCone(target, typename traits::color_type(), false); this->addCone(sources, target, typename traits::color_type());
    };
    void setCone(const typename traits::source_type& source, const typename traits::target_type& target, const typename traits::color_type& color) {
      this->clearCone(target, color, true); this->addCone(source, target, color);
    };
    template<class sourceInputSequence> 
    void setCone(const Obj<sourceInputSequence>& sources, const typename traits::target_type& target, const typename traits::color_type& color){
      this->clearCone(target, color, true); this->addCone(sources, target, color);
    };
    template<class targetInputSequence> 
    void addSupport(const typename traits::source_type& source, const Obj<targetInputSequence >& targets);
    // Unimplemented
    template<class targetInputSequence> 
    void addSupport(const typename traits::source_type& source, const Obj<targetInputSequence>& targets, const typename traits::color_type& color);
    template<typename Sifter_>
    void add(const Obj<Sifter_>& cbg, bool restrict = false) {
      typename ::boost::multi_index::index<typename Sifter_::traits::arrow_container_type::set_type, typename Sifter_::traits::arrowInd>::type& aInd = ::boost::multi_index::get<typename Sifter_::traits::arrowInd>(cbg->_arrows.set);
      
      for(typename ::boost::multi_index::index<typename Sifter_::traits::arrow_container_type::set_type, typename Sifter_::traits::arrowInd>::type::iterator a_iter = aInd.begin(); a_iter != aInd.end(); ++a_iter) {
        this->addArrow(*a_iter, restrict);
      }
      if (!restrict) {
        typename ::boost::multi_index::index<typename Sifter_::traits::base_container_type::set_type, typename Sifter_::traits::baseInd>::type& bInd = ::boost::multi_index::get<typename Sifter_::traits::baseInd>(this->_base.set);
        
        for(typename ::boost::multi_index::index<typename Sifter_::traits::base_container_type::set_type, typename Sifter_::traits::baseInd>::type::iterator b_iter = bInd.begin(); b_iter != bInd.end(); ++b_iter) {
          this->addBasePoint(*b_iter);
        }
        typename ::boost::multi_index::index<typename Sifter_::traits::cap_container_type::set_type, typename Sifter_::traits::capInd>::type& cInd = ::boost::multi_index::get<typename Sifter_::traits::capInd>(this->_cap.set);
        
        for(typename ::boost::multi_index::index<typename Sifter_::traits::cap_container_type::set_type, typename Sifter_::traits::capInd>::type::iterator c_iter = cInd.begin(); c_iter != cInd.end(); ++c_iter) {
          this->addCapPoint(*c_iter);
        }
      }
    };
  }; // class Sifter



  } // namespace ALE_X

#endif
