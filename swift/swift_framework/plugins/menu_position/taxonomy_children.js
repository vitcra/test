(function ($) {

/**
 * Provide the summary information for the taxonomy plugin's vertical tab.
 */
Drupal.behaviors.menuPositionTaxonomyChildren = {
  attach: function (context) {
    $('fieldset#edit-taxonomy-children', context).drupalSetSummary(function (context) {
      if ($('input[name="tc_term"]', context).val()) {
        return Drupal.t('Taxonomy: %term', {'%term' : $('input[name="tc_term"]', context).val()});
      }
      else if ($('select[name="tc_vid"]', context).val() != 0) {
        return Drupal.t('Vocabulary: %vocab', {'%vocab' : $('select[name="tc_vid"] option:selected', context).text()});
      }
      else {
        return Drupal.t('Any taxonomy term');
      }
    });
    // Reset the taxonomy term autocomplete object when the vocabulary changes.
    $('fieldset#edit-taxonomy-children #edit-tc-vid', context).change(function () {
      $input = $('#edit-tc-term');
      // Remove old terms.
      $input.val('');
      // Unbind the original autocomplete handlers.
      $input.unbind('keydown');
      $input.unbind('keyup');
      $input.unbind('blur');
      // Set new autocomplete handlers.
      uri = Drupal.settings.menu_position_taxonomy_url + '/' + $(this).val();
      $('#edit-term-autocomplete').val(uri);
      new Drupal.jsAC($input, new Drupal.ACDB(uri));
    });
  }
};

})(jQuery);
