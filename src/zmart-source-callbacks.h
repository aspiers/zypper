/*---------------------------------------------------------------------\
|                          ____ _   __ __ ___                          |
|                         |__  / \ / / . \ . \                         |
|                           / / \ V /|  _/  _/                         |
|                          / /__ | | | | | |                           |
|                         /_____||_| |_| |_|                           |
|                                                                      |
\---------------------------------------------------------------------*/

#ifndef ZMART_SOURCE_CALLBACKS_H
#define ZMART_SOURCE_CALLBACKS_H

#include <stdlib.h>
#include <iostream>
#include <boost/algorithm/string.hpp>

#include <zypp/base/Logger.h>
#include <zypp/ZYppCallbacks.h>
#include <zypp/Pathname.h>
#include <zypp/KeyRing.h>
#include <zypp/Digest.h>
#include <zypp/Url.h>
#include <zypp/Source.h>

#include "zypper-callbacks.h"

///////////////////////////////////////////////////////////////////
namespace ZmartRecipients
{
///////////////////////////////////////////////////////////////////    
#ifndef LIBZYPP_1xx
    // progress for probing a source
    struct ProbeSourceReceive : public zypp::callback::ReceiveReport<zypp::source::ProbeSourceReport>
    {
      virtual void start(const zypp::Url &url)
      {
        cout << "Determining " << url << " source type..." << endl;
      }
      
      virtual void failedProbe( const zypp::Url &/*url*/, cbstring type )
      {
        cout << ".. not " << type << endl;
      }
      
      virtual void successProbe( const zypp::Url &url, cbstring type )
      {
        cout << url << " is type " << type << endl;
      }
      
      virtual void finish(const zypp::Url &/*url*/, Error error, cbstring reason )
      {
        if ( error == INVALID )
        {
          cout << reason << endl;
          exit(-1);
        }
      }

      virtual bool progress(const zypp::Url &/*url*/, int /*value*/)
      { return true; }

      virtual Action problem( const zypp::Url &/*url*/, Error error, cbstring description )
      {
	display_done ();
	display_error (error, description);
        exit(-1);
        return ABORT;
      }
    };
#endif
    
// progress for downloading a resolvable
struct DownloadResolvableReportReceiver : public zypp::callback::ReceiveReport<zypp::source::DownloadResolvableReport>
{
  zypp::Resolvable::constPtr _resolvable_ptr;
  zypp::Url _url;
  zypp::Pathname _delta;
  zypp::ByteCount _delta_size;
  zypp::Pathname _patch;
  zypp::ByteCount _patch_size;
  
  void display_step( const std::string &what, int value )
  {
    display_progress (what, value);
  }
  
  // Dowmload delta rpm:
  // - path below url reported on start()
  // - expected download size (0 if unknown)
  // - download is interruptable
  // - problems are just informal
  virtual void startDeltaDownload( const zypp::Pathname & filename, const zypp::ByteCount & downloadsize )
  {
      _delta = filename;
      _delta_size = downloadsize;
      std::cerr << "Downloading delta: "
		<< _delta << ", " << _delta_size << std::endl;
  }
  
  virtual bool progressDeltaDownload( int value )
  {
    display_step( "Downloading delta " /*+ _delta.asString()*/, value );
    return true;
  }
  
  virtual void problemDeltaDownload( cbstring description )
  {
    std::cerr << description << std::endl;
  }
  
  virtual void finishDeltaDownload()
  {
    display_done ();
  }
  
  // Apply delta rpm:
  // - local path of downloaded delta
  // - aplpy is not interruptable
  // - problems are just informal
  virtual void startDeltaApply( const zypp::Pathname & filename )
  {
    _delta = filename;
    std::cerr << "Applying delta: " << _delta << std::endl;
  }
  
  virtual void progressDeltaApply( int value )
  {
    display_step( "Applying delta " /* + _delta.asString()*/, value );
  }
  
  virtual void problemDeltaApply( cbstring description )
  {
    std::cerr << description << std::endl;
  }
  
  virtual void finishDeltaApply()
  {
    display_done ();
  }
  
  // Dowmload patch rpm:
  // - path below url reported on start()
  // - expected download size (0 if unknown)
  // - download is interruptable
  virtual void startPatchDownload( const zypp::Pathname & filename, const zypp::ByteCount & downloadsize )
  {
    _patch = filename;
    _patch_size = downloadsize;
    std::cerr << "Downloading patch.rpm: "
	      << _patch << ", " << _patch_size << std::endl;
  }
  
  virtual bool progressPatchDownload( int value )
  {
    display_step( "Applying patchrpm " /* + _patch.asString() */, value );
    return true;
  }
  
  virtual void problemPatchDownload( cbstring description )
  {
    std::cerr << description << std::endl;
  }
  
  virtual void finishPatchDownload()
  {
    display_done ();
  }
  
  
  virtual void start( zypp::Resolvable::constPtr resolvable_ptr, cbUrl url )
  {
    _resolvable_ptr =  resolvable_ptr;
    _url = url;
    std::cerr << "Downloading: " << _resolvable_ptr;
// grr, bad class??
//    zypp::ResObject::constPtr ro =
//      dynamic_pointer_cast<const zypp::ResObject::constPtr> (resolvable_ptr);
    zypp::Package::constPtr ro = zypp::asKind<zypp::Package> (resolvable_ptr);
    if (ro) {
      std::cerr << ", " << ro->archivesize ()
		<< "(" << ro->size () << " unpacked)";
    }
    std::cerr << std::endl;
  }
   
  // return false if the download should be aborted right now
  virtual bool progress(int value, zypp::Resolvable::constPtr /*resolvable_ptr*/)
  {
    display_step( "Downloading " /* + resolvable_ptr->name() */, value );
    return true;
  }

  virtual Action problem( zypp::Resolvable::constPtr resolvable_ptr, Error /*error*/, cbstring description )
  {
    std::cerr << description << std::endl;
   	return (Action) read_action_ari(ABORT);
  }

  virtual void finish( zypp::Resolvable::constPtr /*resolvable_ptr*/, Error error, cbstring reason )
  {
    display_done ();
    display_error (error, reason);
  }
};

#ifndef LIBZYPP_1xx
struct SourceReportReceiver  : public zypp::callback::ReceiveReport<zypp::source::SourceReport>
{     
  virtual void start( zypp::Source_Ref source, cbstring task )
  {
    _task = task;
    _source = source;
    
    display_step(0);
  }
  
  void display_step( int value )
  {
    display_progress ("(" + _source.alias() + ") " + _task , value);
  }
  
  virtual bool progress( int value )
  {
    display_step(value);
    return true;
  }
  
  virtual Action problem( zypp::Source_Ref /*source*/, Error error, cbstring description )
  {
    display_done ();
    display_error (error, description);
    return (Action) read_action_ari ();
  }

  virtual void finish( zypp::Source_Ref /*source*/, cbstring task, Error error, cbstring reason )
  {
    display_step(100);
    // many of these, avoid newline
    if (boost::algorithm::starts_with (task, "Reading patch"))
      cerr_v << '\r' << flush;
    else
      display_done ();
    display_error (error, reason);
  }
  
  std::string _task;
  zypp::Source_Ref _source;
};
#endif

    ///////////////////////////////////////////////////////////////////
}; // namespace ZmartRecipients
///////////////////////////////////////////////////////////////////

class SourceCallbacks {

  private:
#ifndef LIBZYPP_1xx
    ZmartRecipients::ProbeSourceReceive _sourceProbeReport;
    ZmartRecipients::SourceReportReceiver _SourceReport;
#endif
    ZmartRecipients::DownloadResolvableReportReceiver _downloadReport;
  public:
    SourceCallbacks()
    {
#ifndef LIBZYPP_1xx
      _sourceProbeReport.connect();
      _SourceReport.connect();
#endif
      _downloadReport.connect();
    }

    ~SourceCallbacks()
    {
#ifndef LIBZYPP_1xx
      _sourceProbeReport.disconnect();
      _SourceReport.disconnect();
#endif
      _downloadReport.disconnect();
    }

};

#endif 
// Local Variables:
// mode: c++
// c-basic-offset: 2
// End:
