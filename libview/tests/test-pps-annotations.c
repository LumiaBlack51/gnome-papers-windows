// SPDX-License-Identifier: GPL-2.0-or-later
#include <adwaita.h>
#include <glib/gstdio.h>
#include <papers-view.h>

static GtkWidget *
find_child (GtkWidget *parent, const char *type_name)
{
	for (GtkWidget *child = gtk_widget_get_first_child (parent);
	     child;
	     child = gtk_widget_get_next_sibling (child)) {
		if (g_str_equal (G_OBJECT_TYPE_NAME (child), type_name))
			return child;
	}
	return NULL;
}

static void
editing_layers (void)
{
	g_autoptr (GError) error = NULL;
	g_autofree char *uri = g_filename_to_uri (TESTDATADIR "/utf16le-annot.pdf", NULL, &error);
	g_assert_no_error (error);
	g_autoptr (PpsJob) load = pps_job_load_new ();
	pps_job_load_set_uri (PPS_JOB_LOAD (load), uri);
	pps_job_run (load);
	g_assert_true (pps_job_is_succeeded (load, &error));
	g_assert_no_error (error);
	PpsDocument *document = pps_job_load_get_loaded_document (PPS_JOB_LOAD (load));
	g_autoptr (PpsAnnotationModel) tools = pps_annotation_model_new ();
	g_autoptr (PpsDocumentModel) model = g_object_new (PPS_TYPE_DOCUMENT_MODEL,
	                                                 "annotation-model", tools,
	                                                 "continuous", FALSE,
	                                                 "sizing-mode", PPS_SIZING_FREE,
	                                                 "scale", 1.0,
	                                                 NULL);
	g_autoptr (PpsUndoContext) undo = pps_undo_context_new (model);
	g_autoptr (PpsAnnotationsContext) context = pps_annotations_context_new (model, undo);
	PpsView *view = pps_view_new ();
	GtkWidget *window = gtk_window_new ();
	GtkWidget *scrolled = gtk_scrolled_window_new ();
	gtk_window_set_child (GTK_WINDOW (window), scrolled);
	gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (scrolled), GTK_WIDGET (view));
	pps_view_set_model (view, model);
	pps_view_set_annotations_context (view, context);
	pps_document_model_set_document (model, document);
	gtk_widget_realize (window);
	gtk_widget_allocate (scrolled, 800, 1000, -1, NULL);
	gtk_widget_allocate (GTK_WIDGET (view), 800, 1000, -1, NULL);
	GtkWidget *page = find_child (GTK_WIDGET (view), "PpsViewPage");
	g_assert_nonnull (page);

	/* No render completion or resize should be needed to accept editing input. */
	pps_document_model_set_annotation_editing_state (model, PPS_ANNOTATION_EDITING_STATE_INK);
	GtkWidget *ink = find_child (page, "PpsAnnotationLayerInk");
	g_assert_nonnull (ink);
	g_assert_true (gtk_widget_get_visible (ink));
	g_assert_nonnull (gtk_widget_get_cursor (ink));
#ifdef TEST_NO_CURSOR_RESOURCES
	g_assert_cmpstr (gdk_cursor_get_name (gtk_widget_get_cursor (ink)), ==, "crosshair");
#else
	/* The packaged runtime must decode SVG, rather than silently use the fallback. */
	g_assert_null (gdk_cursor_get_name (gtk_widget_get_cursor (ink)));
#endif
	gtk_widget_allocate (page, 600, 800, -1, NULL);
	g_assert_true (gtk_widget_get_can_target (ink));
	g_assert_cmpint (gtk_widget_get_width (ink), ==, 600);
	g_assert_cmpint (gtk_widget_get_height (ink), ==, 800);
	pps_annotation_model_set_tool (tools, TOOL_HIGHLIGHT);
	g_assert_true (gtk_widget_get_visible (ink));
	pps_annotation_model_set_tool (tools, TOOL_ERASER);
	g_assert_true (gtk_widget_get_visible (ink));
	pps_annotation_model_set_tool (tools, TOOL_TEXT);
	GtkWidget *text = find_child (page, "PpsAnnotationLayerObjects");
	g_assert_nonnull (text);
	g_assert_true (gtk_widget_get_visible (text));
	g_assert_false (gtk_widget_get_visible (ink));
	gtk_widget_allocate (page, 600, 800, -1, NULL);
	g_assert_true (gtk_widget_get_can_target (text));
	g_assert_cmpint (gtk_widget_get_width (text), ==, 600);
	g_assert_cmpint (gtk_widget_get_height (text), ==, 800);
	pps_annotation_model_set_tool (tools, TOOL_PENCIL);
	g_assert_false (gtk_widget_get_visible (text));
	g_assert_true (gtk_widget_get_visible (ink));
	pps_document_model_set_annotation_editing_state (model, PPS_ANNOTATION_EDITING_STATE_NONE);
	g_assert_false (gtk_widget_get_visible (text));
	g_assert_false (gtk_widget_get_visible (ink));
	/* Re-entering edit mode must also reuse and show the input layer immediately. */
	pps_document_model_set_annotation_editing_state (model, PPS_ANNOTATION_EDITING_STATE_INK);
	g_assert_true (gtk_widget_get_visible (ink));
	pps_document_model_set_annotation_editing_state (model, PPS_ANNOTATION_EDITING_STATE_NONE);
	gtk_window_destroy (GTK_WINDOW (window));
}

#ifndef TEST_NO_CURSOR_RESOURCES
static void
save_new_annotations (void)
{
	g_autoptr (GError) error = NULL;
	g_autofree char *dir = g_dir_make_tmp ("papers-annotations-XXXXXX", &error);
	g_assert_no_error (error);
	g_autofree char *path = g_build_filename (dir, "annotations.pdf", NULL);
	g_autoptr (GFile) source = g_file_new_for_path (TESTDATADIR "/utf16le-annot.pdf");
	g_autoptr (GFile) target = g_file_new_for_path (path);
	g_assert_true (g_file_copy (source, target, G_FILE_COPY_NONE, NULL, NULL, NULL, &error));
	g_assert_no_error (error);
	g_autofree char *uri = g_file_get_uri (target);
	g_autoptr (PpsJob) load = pps_job_load_new ();
	pps_job_load_set_uri (PPS_JOB_LOAD (load), uri);
	pps_job_run (load);
	g_assert_true (pps_job_is_succeeded (load, &error));
	g_assert_no_error (error);
	PpsDocument *document = pps_job_load_get_loaded_document (PPS_JOB_LOAD (load));
	g_autoptr (PpsAnnotationModel) tools = pps_annotation_model_new ();
	g_autoptr (PpsDocumentModel) model = g_object_new (PPS_TYPE_DOCUMENT_MODEL,
	                                                 "annotation-model", tools,
	                                                 "document", document, NULL);
	g_autoptr (PpsUndoContext) undo = pps_undo_context_new (model);
	g_autoptr (PpsAnnotationsContext) context = pps_annotations_context_new (model, undo);
	pps_job_scheduler_wait ();
	while (g_main_context_iteration (NULL, FALSE)) {}

	PpsPoint start = { 120, 200 }, end = { 220, 200 };
	GdkRGBA color = { 0, 0, 0, 1 };
	PpsAnnotation *text = pps_annotations_context_add_annotation_sync (
	    context, 0, PPS_ANNOTATION_TYPE_FREE_TEXT, &start, &end, &color, NULL);
	g_assert_true (PPS_IS_ANNOTATION_FREE_TEXT (text));
	pps_annotation_set_contents (text, "New text annotation");
	PpsPoint points[] = { { 120, 300 }, { 220, 320 }, { 260, 300 } };
	PpsPath stroke = { points, G_N_ELEMENTS (points) };
	PpsPath *strokes[] = { &stroke };
	PpsInkList ink_list = { strokes, G_N_ELEMENTS (strokes) };
	PpsAnnotationInkAddData data = { .ink_list = &ink_list, .line_width = 2 };
	PpsAnnotation *ink = pps_annotations_context_add_annotation_sync (
	    context, 0, PPS_ANNOTATION_TYPE_INK, &start, &end, &color, &data);
	g_assert_true (PPS_IS_ANNOTATION_INK (ink));
	g_assert_true (pps_document_annotations_document_is_modified (PPS_DOCUMENT_ANNOTATIONS (document)));
	g_autoptr (PpsJob) save = pps_job_save_new (document, uri, uri);
	pps_job_run (save);
	g_assert_true (pps_job_is_succeeded (save, &error));
	g_assert_no_error (error);
	g_autoptr (PpsDocument) reopened = pps_document_factory_get_document (uri, &error);
	g_assert_no_error (error);
	g_assert_true (pps_document_load (reopened, uri, &error));
	g_assert_no_error (error);
	g_autoptr (PpsPage) saved_page = pps_document_get_page (reopened, 0);
	GList *annots = pps_document_annotations_get_annotations (PPS_DOCUMENT_ANNOTATIONS (reopened), saved_page);
	gboolean found_text = FALSE, found_ink = FALSE;
	for (GList *a = annots; a; a = a->next) {
		if (PPS_IS_ANNOTATION_FREE_TEXT (a->data))
			found_text |= g_strcmp0 (pps_annotation_get_contents (a->data), "New text annotation") == 0;
		if (PPS_IS_ANNOTATION_INK (a->data)) {
			PpsInkList *saved_ink = pps_annotation_ink_get_ink_list (a->data);
			g_assert_cmpuint (saved_ink->n_paths, ==, 1);
			g_assert_cmpuint (saved_ink->paths[0]->n_points, ==, G_N_ELEMENTS (points));
			found_ink = TRUE;
		}
	}
	g_assert_true (found_text);
	g_assert_true (found_ink);
	g_assert_cmpint (g_unlink (path), ==, 0);
	g_assert_cmpint (g_rmdir (dir), ==, 0);
}
#endif

int
main (int argc, char *argv[])
{
	g_test_init (&argc, &argv, NULL);
	if (!gtk_init_check ())
		return 77;
	adw_init ();
	g_assert_true (pps_init ());
	g_test_add_func ("/libview-annotations/editing-layers", editing_layers);
#ifndef TEST_NO_CURSOR_RESOURCES
	g_test_add_func ("/libview-annotations/save-new-annotations", save_new_annotations);
#endif
	return g_test_run ();
}
