/**
 * 
 */
package spade.edge.opm;

import spade.core.AbstractEdge;
import spade.vertex.opm.Artifact;

/**
 * @author cshou
 *
 */
public class WasTransferredFrom extends AbstractEdge {

	/**
	 * @param generatedArtifact
	 * @param remoteArtifact
	 */
	public WasTransferredFrom(Artifact generatedArtifact, Artifact remoteArtifact) {
		setSourceVertex(generatedArtifact);
        setDestinationVertex(remoteArtifact);
        addAnnotation("type", "WasTransferredFrom");
	}
	
}
