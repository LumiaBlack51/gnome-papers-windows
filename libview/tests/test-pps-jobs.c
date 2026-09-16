#include <glib.h>
#include <glib/gstdio.h>
#include <papers-document.h>
#include <pps-document.h>
#include <pps-init.h>
#include <pps-job.h>
#include <pps-jobs.h>

static void
get_annots (void)
{
	PpsDocument *document;
	PpsJob *job = pps_job_load_new ();
	gchar *file_path = TESTDATADIR "/utf16le-annot.pdf";
	GFile *file = NULL;
	gchar *uri = NULL;
	gboolean hasBackend;
	gint n_pages;
	GList *annots;
	PpsPage *page;

	file = g_file_new_for_path (file_path);
	uri = g_file_get_uri (file);
	hasBackend = pps_init ();

	g_assert_nonnull (file);
	g_assert_nonnull (uri);
	g_assert_nonnull (job);
	g_assert_true (hasBackend);

	pps_job_load_set_uri (PPS_JOB_LOAD (job), uri);

	pps_job_run (job);

	document = pps_job_load_get_loaded_document (PPS_JOB_LOAD (job));
	n_pages = pps_document_get_n_pages (document);

	g_assert_nonnull (document);
	g_assert (n_pages == 1);

	page = pps_document_get_page (document, 0);
	annots = pps_document_annotations_get_annotations (PPS_DOCUMENT_ANNOTATIONS (document),
	                                                   page);

	g_assert (g_list_length (annots) == 2);
}

static void
load_encrypted (void)
{
	PpsDocument *document;
	PpsJob *job = pps_job_load_new ();
	gchar *file_path = TESTDATADIR "/PasswordEncrypted.pdf";
	GFile *file = NULL;
	gchar *uri = NULL;
	gboolean hasBackend;
	gint n_pages;

	file = g_file_new_for_path (file_path);
	uri = g_file_get_uri (file);
	hasBackend = pps_init ();

	g_assert_nonnull (file);
	g_assert_nonnull (uri);
	g_assert_nonnull (job);
	g_assert_true (hasBackend);

	pps_job_load_set_uri (PPS_JOB_LOAD (job), uri);

	pps_job_run (job);

	pps_job_reset (job);

	pps_job_load_set_password (PPS_JOB_LOAD (job), "password");

	pps_job_run (job);

	document = pps_job_load_get_loaded_document (PPS_JOB_LOAD (job));
	n_pages = pps_document_get_n_pages (document);

	g_assert_nonnull (document);
	g_assert (n_pages == 1);
}

/* Exercise the same JobSave path as Ctrl+S with a real PDF backend. */
static void
save_in_place (void)
{
	g_autoptr (GError) error = NULL;
	g_autofree gchar *dir = g_dir_make_tmp ("papers-save-test-XXXXXX", &error);
	g_assert_no_error (error);
	g_autofree gchar *path = g_build_filename (dir, "save-test.pdf", NULL);
	g_autoptr (GFile) source = g_file_new_for_path (TESTDATADIR "/utf16le-annot.pdf");
	g_autoptr (GFile) target = g_file_new_for_path (path);
	g_assert_true (g_file_copy (source, target, G_FILE_COPY_NONE, NULL, NULL, NULL, &error));
	g_assert_no_error (error);
	g_autofree gchar *uri = g_file_get_uri (target);
	g_assert_true (pps_init ());
	g_autoptr (PpsDocument) document = pps_document_factory_get_document (uri, &error);
	g_assert_no_error (error);
	g_assert_nonnull (document);
	g_assert_true (pps_document_load (document, uri, &error));
	g_assert_no_error (error);
	g_autoptr (PpsPage) page = pps_document_get_page (document, 0);
	GList *annots = pps_document_annotations_get_annotations (PPS_DOCUMENT_ANNOTATIONS (document), page);
	g_assert_nonnull (annots);
	PpsAnnotation *annot = annots->data;
	for (int i = 0; i < 2; i++) {
		const char *text = i == 0 ? "First in-place save" : "Second in-place save";
		pps_annotation_set_contents (annot, text);
		g_autoptr (PpsJob) save = pps_job_save_new (document, uri, uri);
		pps_job_run (save);
		gboolean succeeded = pps_job_is_succeeded (save, &error);
		g_assert_no_error (error);
		g_assert_true (succeeded);
		g_autoptr (PpsDocument) reopened = pps_document_factory_get_document (uri, &error);
		g_assert_no_error (error);
		g_assert_true (pps_document_load (reopened, uri, &error));
		g_assert_no_error (error);
		g_assert_cmpint (pps_document_get_n_pages (reopened), ==, 1);
		g_autoptr (PpsPage) saved_page = pps_document_get_page (reopened, 0);
		GList *saved = pps_document_annotations_get_annotations (PPS_DOCUMENT_ANNOTATIONS (reopened), saved_page);
		gboolean found = FALSE;
		for (GList *a = saved; a; a = a->next)
			found |= g_strcmp0 (pps_annotation_get_contents (a->data), text) == 0;
		g_assert_true (found);
	}
	/* An invalid destination must fail without damaging the last saved file. */
	g_autofree gchar *before = NULL;
	gsize before_len;
	g_assert_true (g_file_get_contents (path, &before, &before_len, &error));
	g_autofree gchar *bad_path = g_build_filename (dir, "missing", "save.pdf", NULL);
	g_autofree gchar *bad_uri = g_filename_to_uri (bad_path, NULL, &error);
	g_autoptr (PpsJob) failed = pps_job_save_new (document, bad_uri, uri);
	pps_job_run (failed);
	g_assert_false (pps_job_is_succeeded (failed, &error));
	g_clear_error (&error);
	g_autofree gchar *after = NULL;
	gsize after_len;
	g_assert_true (g_file_get_contents (path, &after, &after_len, &error));
	g_assert_cmpmem (before, before_len, after, after_len);
	g_clear_object (&page);
	g_clear_object (&document);
	g_assert_cmpint (g_unlink (path), ==, 0);
	g_assert_cmpint (g_rmdir (dir), ==, 0);
}

gint
main (gint argc,
      gchar *argv[])
{
	g_test_init (&argc, &argv, NULL);
	g_test_add_func ("/libview-jobs/get_annots", get_annots);
	g_test_add_func ("/libview-jobs/save-in-place", save_in_place);
	g_test_add_func ("/libview-jobs/load_encrypted", load_encrypted);
	return g_test_run ();
}
