import java.awt.BorderLayout;
import java.awt.Color;
import java.awt.Graphics;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.MouseEvent;
import java.awt.event.MouseListener;
import java.awt.event.MouseMotionListener;
import java.time.ZoneId;
import java.util.Vector;

import javax.swing.*;

/** La classe ZoneSaisirPointsAfficherSegments. */
public class ZoneSaisirPointsAfficherSegments extends JPanel  {

    public static boolean enveloppeConvexe = false;
    public static boolean monotoneTriangulation = false;
    public static boolean intersectionSegments = false;

    /** Creation de la zone d'affichage. */
    public ZoneSaisirPointsAfficherSegments()
    {
        // Le canvas d'affichage
        final CanvasSaisirPointsAfficherSegments canvas = new CanvasSaisirPointsAfficherSegments();

        // Panel des boutons
        JPanel panelBoutons = new JPanel();


        // Creation du bouton Rand
        final JTextField textNombrePoint = new JTextField("50");
        textNombrePoint.setColumns(5);

        // Creation du bouton Effacer
        JButton effacer = new JButton("Effacer");
        JButton triangulation = new JButton("Triangulation Monotone");
        JButton envConvexe = new JButton("Enveloppe Conv.");
        JButton rand = new JButton("Rand");
        JButton intersect = new JButton("Intersection de Segments");

        // Action du bouton intersection de segments
        intersect.addActionListener( new ActionListener(){
                                         public void actionPerformed(ActionEvent evt) {
                                             // Suppression des points et des segments
                                             //canvas.points.removeAllElements();

                                             //canvas.points.removeAllElements();
                                             canvas.segments.removeAllElements();

                                             enveloppeConvexe = false;
                                             monotoneTriangulation = false;
                                             intersectionSegments = true;

                                             canvas.calculer();
                                             canvas.repaint();
                                         }
                                     }
        );

        // Action du bouton Triangulation Monotone
        triangulation.addActionListener( new ActionListener(){
                                          public void actionPerformed(ActionEvent evt) {
                                              // Suppression des points et des segments
                                              //canvas.points.removeAllElements();

                                              canvas.segments.removeAllElements();

                                              enveloppeConvexe = false;
                                              monotoneTriangulation = true;
                                              intersectionSegments = false;

                                              canvas.calculer();
                                              canvas.repaint();
                                          }
                                      }
        );

        // Action du bouton EnveloppeConvexe
        envConvexe.addActionListener( new ActionListener(){
                                    public void actionPerformed(ActionEvent evt) {
                                        // Suppression des points et des segments
                                        //canvas.points.removeAllElements();
                                        for(int i=0; i < canvas.points.size(); ++i){
                                            canvas.points.elementAt(i).isRight = true;
                                        }
                                        canvas.segments.removeAllElements();

                                        enveloppeConvexe = true;
                                        monotoneTriangulation = false;
                                        intersectionSegments = false;

                                        canvas.calculer();
                                        canvas.repaint();
                                    }
                                }
        );

        // Action du bouton Effacer
        effacer.addActionListener( new ActionListener(){
                                       public void actionPerformed(ActionEvent evt) {
                                           // Suppression des points et des segments
                                           canvas.points.removeAllElements();
                                           canvas.segments.removeAllElements();

                                           enveloppeConvexe = false;
                                           monotoneTriangulation = false;
                                           intersectionSegments = false;

                                           canvas.repaint();
                                       }
                                   }
        );



        // Action du bouton Rand
        rand.addActionListener( new ActionListener(){
                                    public void actionPerformed(ActionEvent evt) {
                                        // Suppression des points et des segments
                                        canvas.points.removeAllElements();
                                        canvas.segments.removeAllElements();

                                        enveloppeConvexe = false;
                                        monotoneTriangulation = false;
                                        intersectionSegments = false;

                                        int n = Integer.parseInt(textNombrePoint.getText());
                                        for (int i = 0; i < n; i++)
                                        {
                                            canvas.points.addElement(
                                                    new Point(
                                                            2 + Algorithmes.rand(canvas.getWidth()-4),
                                                            2 + Algorithmes.rand(canvas.getHeight()-4)
                                                    )
                                            );
                                        }
                                        canvas.calculer();
                                        canvas.repaint();
                                    }
                                }
        );

        // Ajout des boutons au panel panelBoutons
        panelBoutons.add(effacer);
        panelBoutons.add(triangulation);
        panelBoutons.add(envConvexe);
        panelBoutons.add(intersect);
        panelBoutons.add(rand);
        panelBoutons.add(textNombrePoint);
        setLayout(new BorderLayout());

        // Ajout du canvas au centre
        add(canvas, BorderLayout.CENTER);

        // Ajout des boutons au nord
        add(panelBoutons, BorderLayout.SOUTH);
    }
}

/** La classe CanvasSaisirPointsAfficherSegments. */
class CanvasSaisirPointsAfficherSegments extends JPanel implements MouseListener, MouseMotionListener {
    /** La liste des points affiches. */
    Vector<Point> points;

    /** La liste des segments affiches. */
    Vector<Segment> segments;

    /** Le numero du point selectionne. */
    private int numSelectedPoint;

    /** La couleur d'un point a l'ecran. */
    private final Color pointColor = Color.GRAY;

    /** La couleur d'un segment a l'ecran. */
    private final Color segmentColor = Color.BLUE;

    /** La couleur d'un point selectionne a l'ecran. */
    private final Color selectedPointColor = Color.RED;

    /** La taille d'un point a l'ecran. */
    private final int POINT_SIZE = 2;

    /** Creation de la zone d'affichage. */
    public CanvasSaisirPointsAfficherSegments()
    {
        // Creation du vecteur de points
        points = new Vector<Point>();

        // Creation du vecteur de segments
        segments = new Vector<Segment>();

        // Initialisation du point selectionne
        numSelectedPoint = -1;

        // Initialisation de la couleur de fond
        setBackground(Color.WHITE);

        // Ajout de la gestion des actions souris
        this.addMouseListener(this);
        this.addMouseMotionListener(this);
    }

    /** Dessin de la zone d'affichage. */
    public void paint(Graphics g) {
        // Efface le fond
        g.clearRect(0,0,getWidth(),getHeight());

        // Dessin des segments
        drawSegments(g);

        // Dessin des points
        drawPoints(g);
    }

    /** Affichage des points. */
    private void drawPoints(Graphics g) {
        for (int n = 0; n < points.size(); n++) {
            Point p = points.elementAt(n);

            if ( n == numSelectedPoint )
                g.setColor(selectedPointColor);
            else
                g.setColor(pointColor);

            g.fillOval((int)(p.x - POINT_SIZE), (int)(p.y - POINT_SIZE), 2 * POINT_SIZE + 1, 2 * POINT_SIZE + 1);
            g.drawOval((int)(p.x - 2 * POINT_SIZE), (int)(p.y - 2 * POINT_SIZE), 2 * 2 * POINT_SIZE,	2 * 2 * POINT_SIZE);

            if( ZoneSaisirPointsAfficherSegments.monotoneTriangulation ){
                if(p.isRight) g.drawString("D", (int)p.x + 10, (int)p.y + 10);
                else g.drawString("G", (int)p.x - 20, (int)p.y + 10);

                g.drawString(Integer.toString(p.number), (int)p.x + 30, (int)p.y + 10);
            }
            else if( ZoneSaisirPointsAfficherSegments.intersectionSegments ){

            }
            else{
                g.drawString(Double.toString(p.x), (int)p.x-15, (int)p.y+20);
            }
        }
    }

    /** Affichage des segments. */
    private void drawSegments(Graphics g) {
        for (int n = 0; n < segments.size(); n++) {
            Segment segment = segments.elementAt(n);

            if(segment.diagonale == true)
                g.setColor(Color.pink);
            else
                g.setColor(segmentColor);

            g.drawLine((int)segment.a.x,(int)segment.a.y,(int)segment.b.x,(int)segment.b.y);
            if( ZoneSaisirPointsAfficherSegments.intersectionSegments ){
                g.drawString(Integer.toString( segment.number ), (int)segment.a.x + 10, (int)segment.a.y + 5);
            }

        }
    }

    /** Retourne le numero du point situe en (x,y). */
    private int getNumSelectedPoint(int x, int y) {
        for(int n = 0; n < points.size(); n++)
        {
            Point p = points.elementAt(n);
            if
                    (
                    p.x > x - 2 * POINT_SIZE &&
                            p.x < x + 2 * POINT_SIZE &&
                            p.y > y - 2 * POINT_SIZE &&
                            p.y < y + 2 * POINT_SIZE
                    )
                return n;
        }
        return -1;
    }

    /** Un point est ajoute si on presse le bouton de gauche
     * 	et si aucun point n'est selectionne.
     * 	Un point est suuprime si on presse un autre bouton
     * 	et si un point est selectionne.
     */
    public void mousePressed(MouseEvent evt) {
        if ( evt.getButton() == 1 )
        {
            if (numSelectedPoint == -1)
            {
                numSelectedPoint = points.size();
                points.addElement(new Point(evt.getX(), evt.getY()));
                calculer();
                repaint();
            }
        }
        else
        {
            if (numSelectedPoint != -1)
            {
                points.removeElementAt(numSelectedPoint);
                numSelectedPoint = getNumSelectedPoint(evt.getX(), evt.getY());
                calculer();
                repaint();
            }
        }
    }

    /** Le x et y du point numSelectedPoint est modifie si
     * 	la souris change de position avec un bouton enfonce.
     */
    public void mouseDragged(MouseEvent evt) {
        if (numSelectedPoint != -1)
        {
            points.elementAt(numSelectedPoint).x = evt.getX();
            points.elementAt(numSelectedPoint).y = evt.getY();
            calculer();
            repaint();
        }
    }

    /** Le numSelectedPoint est calcule si
     * 	la souris change de position sans bouton enfonce.
     */
    public void mouseMoved(MouseEvent evt) {
        numSelectedPoint = getNumSelectedPoint(evt.getX(), evt.getY());
        repaint();
    }

    /** Lance l'algorithme sur l'ensemble de points. */
    public void calculer()
    {
        if( ZoneSaisirPointsAfficherSegments.monotoneTriangulation ){
            segments = Algorithmes.algorithme1(points);
        }
        else if( ZoneSaisirPointsAfficherSegments.enveloppeConvexe ){
            segments = Algorithmes.quickHull(points);
        }
        else if(ZoneSaisirPointsAfficherSegments.intersectionSegments ){
            segments = Algorithmes.segmentsIntersection(points);
        }
    }

    public void mouseReleased(MouseEvent evt) {}
    public void mouseEntered(MouseEvent evt) {}
    public void mouseExited(MouseEvent evt) {}
    public void mouseClicked(MouseEvent evt) {}
}
