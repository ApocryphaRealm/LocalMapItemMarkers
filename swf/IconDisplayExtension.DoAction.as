var extraMarkerContainer = this;
var extraMarkers = new Array();
var CREATE_ICONTYPE = 1;
var CREATE_STRIDE = 3;
_parent.IconDisplay.ExtraMarkerData = new Array();
_parent.IconDisplay.PostCreateMarkers = function(a_showEnemyMarkers, a_showHostileMarkers, a_showGuardMarkers, a_showDeadMarkers, a_showTeammateMarkers, a_showNeutralMarkers)
{
   if(this.GetCreatingMarkers())
   {
      return undefined;
   }
   var _em = LocalMap.ExtraMapMarker;
   if(_em.IconTypes.length < 10)
   {
      while(_em.IconTypes.length < 10)
      {
         _em.IconTypes.push("NeutralMarker");
      }
   }
   if(_em.ExtraMarkerTints == undefined)
   {
      _em.ExtraMarkerTints = new Array(16744234,6990335,13148747,8313175);
   }
   var _loc8_;
   while(extraMarkers.length)
   {
      _loc8_ = extraMarkers.pop();
      _loc8_.removeMovieClip();
   }
   var _loc9_ = this._markerList;
   if(_loc9_ == undefined || _loc9_.length == 0)
   {
      _loc9_ = this.Markers;
      if(_loc9_ == undefined || _loc9_.length == 0)
      {
         return undefined;
      }
   }
   var _loc10_ = _loc9_.length;
   var _loc11_ = 0;
   var _loc12_ = 0;
   var _loc13_;
   var _loc14_;
   var _loc15_;
   var _loc16_;
   var _loc17_;
   var _loc18_;
   while(_loc12_ < _loc10_)
   {
      _loc13_ = this.MarkerData[_loc12_ * CREATE_STRIDE + CREATE_ICONTYPE];
      if(_loc13_ == 0)
      {
         _loc14_ = _loc9_[_loc12_];
         _loc16_ = this.ExtraMarkerData[_loc11_];
         _loc15_ = LocalMap.ExtraMapMarker.IconTypes[_loc16_];
         _loc11_ += 1;
         _loc8_ = extraMarkerContainer.attachMovie(_loc15_,"ExtraMarker" + _loc12_,getNextHighestDepth());
         if(_loc16_ >= 6)
         {
            _loc17_ = LocalMap.ExtraMapMarker.ExtraMarkerTints[_loc16_ - 6];
            if(_loc17_ != undefined)
            {
               _loc18_ = new Object();
               _loc18_.rgb = _loc17_;
               _loc8_.transform.colorTransform = new flash.geom.ColorTransform(0,0,0,1,_loc17_ >> 16 & 0xFF,_loc17_ >> 8 & 0xFF,_loc17_ & 0xFF,0);
            }
         }
         extraMarkers.push(_loc8_);
         _loc9_[_loc12_] = _loc8_;
         _loc14_.removeMovieClip();
      }
      _loc12_ += 1;
   }
   this.MarkerDescriptionObj._parent.swapDepths(getNextHighestDepth());
};
_global.gfxExtensions = true;
GlobalFunc.SetLockFunction();
