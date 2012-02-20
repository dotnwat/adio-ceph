/*
    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/
/*
 * File         : Errhandler.java
 * Author       : Xinying Li
 * Created      : Thu Apr  9 12:22:15 1998
 * Revision     : $Revision: 1.3 $
 * Updated      : $Date: 2001/08/07 16:36:25 $
 * Copyright: Northeast Parallel Architectures Center
 *            at Syracuse University 1998
 */

package mpi;
//import mpi.*;

public class Errhandler{
  public final static int FATAL = 1;
  public final static int RETURN = 0;

  private static native void init();

  //public Errhandler() {}
  public Errhandler(int Type) { GetErrhandler(Type);}
  public Errhandler(long _handle) { handle = _handle;}  

  protected native void GetErrhandler(int Type);
  
  protected long handle;

  static {
    init();
  }

}
